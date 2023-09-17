#include "Service.h"
#include "RPC.h"
#include <sstream>
#include <arpa/inet.h>
// #include "../pb/tp.pb.h"
extern "C"
{
#include "skynet_server.h"
#include "skynet.h"
#include "skynet_handle.h"
#include "skynet_harbor.h"
}
#define MAX_TIMER_IDX 0x7fffffff


#include <execinfo.h>
void print_stacktrace()
{
	int size = 16;
	void * array[16];
	int stack_num = backtrace(array, size);
	char ** stacktrace = backtrace_symbols(array, stack_num);
	for (int i = 0; i < stack_num; ++i)
	{
		LOG(INFO) << stacktrace[i];
	}
	free(stacktrace);
}

Service::Service()
	:
	m_service_context(),
	m_dsp(&m_service_context)
{
	m_ctx = NULL;
	m_service_started = false;
	m_parent = skynet_current_handle();
	//LOG(INFO) << "service() parent:" << std::hex << m_parent;
	m_process_uid = 0;

	m_source = 0;
	m_session = 0;
	m_msg_type = 0;
	m_service_roomid = 0;
}

Service::~Service()
{
}

void Service::service_send(const Message& msg, uint32_t handle)
{
	//LOG(INFO) << "service_send m_process_uid = " << m_process_uid;
	service_fsend(msg, handle, 0, m_process_uid, SUBTYPE_PROTOBUF);
}

void Service::service_send(const Message& msg, uint32_t handle, int64_t uid)
{
	//LOG(INFO) << "service_send uid = " << uid;
	service_fsend(msg, handle, 0, uid, SUBTYPE_PROTOBUF);
}

void Service::service_fsend(const Message& msg, uint32_t handle, uint32_t source, int64_t uid, uint32_t type)
{
	char* data;
	uint32_t size;
	int32_t roomid = get_service_roomid();
	//serialize_imsg_type(msg, data, size, uid, SUBTYPE_PROTOBUF, roomid);
	serialize_imsg_type(msg, data, size, uid, type, roomid);

	
	skynet_send_noleak(m_ctx, source, handle, PTYPE_TEXT | PTYPE_TAG_DONTCOPY, 0, data, size);

	//LOG(INFO) << "service_fsend PACK: " << msg.GetTypeName() << " " << size;

}

void Service::service_system_send(const Message& msg, uint32_t handle)
{
	//LOG(INFO) << "service_system_send m_process_uid = " << m_process_uid;
	service_fsend(msg, handle, 0, m_process_uid, SUBTYPE_SYSTEM);
}

void Service::service_system_send(const Message& msg, uint32_t handle, int64_t uid)
{
	//LOG(INFO) << "service_system_send uid = " << uid;
	service_fsend(msg, handle, 0, uid, SUBTYPE_SYSTEM);
}

void Service::service_lua_rsp(const Message& msg)
{
	service_lua_rsp(msg, m_process_uid);
}

void Service::service_lua_rsp(const Message& msg, int64_t uid)
{
	service_lua_frsp(msg, uid, m_source, m_session);
}


void Service::service_lua_frsp(const Message& msg, int64_t uid, uint32_t destination, int session)
{
	char* data;
	uint32_t size;
	int32_t roomid = get_service_roomid();
	serialize_imsg_type(msg, data, size, uid, SUBTYPE_PROTOBUF, roomid);

	LOG(INFO) << "service_lua_frsp destination= " << destination << ", session =  " << session;
	skynet_send_noleak(m_ctx, skynet_context_handle(m_ctx), destination, PTYPE_RESPONSE | PTYPE_TAG_DONTCOPY, session, data, size);
}


bool Service::service_init(skynet_context* ctx, const void* parm, int len)
{
	m_service_context.init_service_context(ctx);

	m_ctx = ctx;
	timer_init(this);
	rpc_init_client(&m_service_context);
	rpc_init_server(&m_service_context);

	string sparm;
	bool nostart = false;

	if (parm != NULL)
		sparm.assign((const char*)parm, len);
	string tparm;
	string temp;
	tie(temp, tparm) = divide_string(sparm, ' ');

	if (temp == "nostart") //启动init后就结束了
	{
		LOG(INFO) << "service nostart.";
		nostart = true;
	}
	else
	{
		tparm = sparm;
	}

	// service_callback(pb::iNameListBRC::descriptor()->full_name(), boost::bind(&Service::proto_service_name_brc, this, _1, _2));

	auto ret = m_init_func.call(tparm);

	if (nostart)
		return false;

	return ret;
}

void Service::service_callback(const string& name, const DispatcherT<uint32_t>::Callback& func)
{
	LOG(INFO) << "service_callback name = " << name;
	m_dsp.register_callback(name, func);
}

void Service::service_over()
{
	m_service_context.service_over_event();
	auto handle = skynet_context_handle(m_ctx);
	// if (m_parent != 0)
	// {
	// 	pb::iKillService req;
	// 	req.set_handle(handle);
	// 	service_send(req, m_parent);
	// }

	LOG(INFO) << "retire handle:" << std::hex << handle;
	if (skynet_handle_retire(handle) != 1)
		LOG(ERROR) << "retire handle error. handle:" << std::hex << handle;

}

void Service::service_depend_on(const string& name)
{
	m_named_service[name] = 0;
}

void Service::service_poll(const char* data, uint32_t size, uint32_t source, int session, int type)
{
    m_source = source;
    m_session = session;
    m_msg_type = type;
    m_msg_data.assign(data, size);

    switch (type)
    {
    case PTYPE_TEXT:
    {
        uint32_t sub_type;
        memcpy(&sub_type, data, sizeof(uint32_t));
        //LOG(INFO) << "sub_type = " << sub_type;
        sub_type = ntohl(sub_type);
        //LOG(INFO) << "convert sub_type = " << sub_type;

        data += sizeof(uint32_t);
        size -= sizeof(uint32_t);
        if (sub_type == SUBTYPE_PROTOBUF || sub_type == SUBTYPE_SYSTEM) {//单项的消息传递
            //LOG(INFO) << "SUBTYPE_PROTOBUF";
            proto(data, size, source); // 转发给之前注册的proto协议
        }
        else if (sub_type == SUBTYPE_RPC_SERVER) {
            
            m_process_uid = get_uid_from_stream(data);
            //LOG(INFO) << "SUBTYPE_RPC_SERVER m_process_uid = " << m_process_uid;
            rpc_event_server(data, size, source, session);
            m_process_uid = 0;
        }
        else if(sub_type == SUBTYPE_PLAIN_TEXT){
            //LOG(INFO) << "SUBTYPE_PLAIN_TEXT";
            string stream(data, size);
            string from;
            tie(from, stream) = divide_string(stream, ' ');
            //LOG(INFO) << "SUBTYPE_PLAIN_TEXT from = " << from;
            if (!stream.empty() && from == "php")
            {
                string uid;
                tie(uid, stream) = divide_string(stream, ' ');
                //LOG(INFO) << "uid  = " << uid;
                if (!stream.empty())
                {
                    LOG(WARNING) << "uid = " << uid;
                    m_process_uid = atoll(uid.c_str());
                    LOG(WARNING) << "m_process_uid = " << m_process_uid;
                }
            }
            //LOG(INFO) << "text_message source = " << source << " session = " << session;
            text_message(data, size, source, session);
            m_process_uid = 0;
        }
        
        break;
    }

    case PTYPE_CLIENT:
    {
        //LOG(INFO) << "Service::service_poll PTYPE_CLIENT";
        message(data, size, source);
        break;
    }

    case PTYPE_RESPONSE: // Timer and RPC RESPONSE 可以根据source来判断是rpc还是timer
    {
        LOG(INFO) << "PTYPE_RESPONSE source = " << source << " m_parent =  " << m_parent;
        if (source == 0) {
            timer_timeout(session);
        }
        else {
            uint32_t sub_type;
            memcpy(&sub_type, data, sizeof(uint32_t));
            //LOG(INFO) << "PTYPE_RESPONSE sub_type = " << sub_type;
            sub_type = ntohl(sub_type);
            //LOG(INFO) << "PTYPE_RESPONSE convert sub_type = " << sub_type;

            data += sizeof(uint32_t);
            size -= sizeof(uint32_t);

            if (sub_type == SUBTYPE_RPC_CLIENT) {
                m_process_uid = get_uid_from_stream(data);
                //LOG(INFO) << "SUBTYPE_RPC_CLIENT m_process_uid = " << m_process_uid;
                // rpc_event_client(data, size, source, session);
                rpc_event_client_proxy(data, size);
                m_process_uid = 0;
            }
            
        }
        
        break;
    }

    default:
        break;
    }

    m_source = 0;
    m_session = 0;
}



void Service::proto(const char* data, uint32_t size, uint32_t source)
{
    m_process_uid = get_uid_from_stream(data);

    //LOG(INFO) << "proto m_process_uid = " << m_process_uid;

    DispatcherStatus status = m_dsp.dispatch_message(data, size, source);
    if (status == DISPATCHER_PACK_ERROR || status == DISPATCHER_PB_ERROR)
    {
        LOG(ERROR) << "inner proto to service error";
    }

    if (status == DISPATCHER_CALLBACK_ERROR)
    {
        InPack pack;
        pack.inner_reset(data, size);
        LOG(WARNING) << "dispatch error. service name:" << m_service_name << " pb name:" << pack.m_type_name;
    }
    m_process_uid = 0;
}

void Service::text_response(const string& rsp)
{
	// API 被设计成传递一个 C 指针和长度，而不是经过当前消息的 pack 函数打包。或者你也可以省略 size 而传入一个字符串
	skynet_send_noleak(m_ctx, 0, m_source, PTYPE_RESPONSE, m_session, (void*)rsp.c_str(), rsp.size());
}

bool Service::is_depend_ok()
{
	bool ret = true;
	std::map<std::string, uint32_t>::iterator it = m_named_service.begin();
	for (; it != m_named_service.end(); ++it)
	{
		if (it->second == 0)
		{
			ret = false;
			break;
		}
	}
	return ret;
}

uint32_t Service::new_skynet_service(const string& name, const string& param)
{
	// 创建skynet_context
	skynet_context* ctx = skynet_context_new(name.c_str(), param.c_str());
	uint32_t handle = 0;
	if (ctx)
	{
		handle = skynet_context_handle(ctx);
		// 把服务添加到当前的服务上
		add_child(handle);
	}
	else
	{
		LOG(ERROR) << "name = " << name << " param =  " << param;
		LOG(ERROR) << "create master element error";
	}
	return handle;
}


// void Service::proto_service_name_brc(Message* data, uint32_t handle)
// {
// 	// Service接受iNameList的消息

// 	// name_got_event
// 	// sevice_start
// 	// broadcast child service

// 	pb::iNameListBRC* msg = dynamic_cast<pb::iNameListBRC*>(data);
// 	LOG(INFO) << "pb::iNameListBRC* msg = " << msg->ShortDebugString();
//     std::map<std::string, uint32_t> new_up_names;
//     std::string up_name = msg->up_name();
// 	for (int i = 0; i < msg->list_size(); ++i)
// 	{
// 		const string& name = msg->list(i).name();
// 		LOG(INFO) << "name = " << name;
// 		LOG(INFO) << "address = " << msg->list(i).address();
// 		std::map<std::string, uint32_t>::iterator it = m_named_service.find(name);
// 		if (it != m_named_service.end())
// 		{
//             uint32_t new_handle = msg->list(i).address();
//             if (0 == it->second || it->second != new_handle || up_name == name)
//             {
//                 new_up_names[name] = new_handle;
//             }
// 			it->second = msg->list(i).address();
// 			LOG(INFO) << "proto_service_name_brc name = " << name;
// 			//name_got_event(name);
// 		}
// 		else
// 		{
// 			m_named_service[name] = msg->list(i).address();
// 		}
// 	}

//     for (auto& item : new_up_names)
//     {
//         std::string name = item.first;
//         LOG(INFO) << "proto_service_name_brc new up name = " << name;
//         name_got_event(name);
//     }

// 	if (is_depend_ok() && !m_service_started)
// 	{
// 		// 开启服务
// 		m_service_started = true;
// 		LOG(INFO) << "service [" << m_service_name << "] start.";
// 		service_start();
// 	}

// 	//广播给子服务
// 	std::set<uint32_t>::iterator it = m_children.begin();
// 	for (; it != m_children.end(); ++it)
// 	{
// 		service_send(*msg, *it);
// 	}
// }
