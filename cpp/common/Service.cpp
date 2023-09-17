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
		InPack pack;
		if (!pack.inner_reset(data, size))
		{
			LOG(ERROR) << "dispatch message pack error";
			return;
		}
        uint32_t sub_type = pack.get_sub_type();
		LOG(INFO) << "sub_type:" << sub_type << " pbname:" << pack.get_pbname();
        if (sub_type == SUBTYPE_PROTOBUF || sub_type == SUBTYPE_SYSTEM) {//单项的消息传递
            //LOG(INFO) << "SUBTYPE_PROTOBUF";
            proto(pack, source); // 转发给之前注册的proto协议
        }
        else if (sub_type == SUBTYPE_RPC_SERVER) {
            m_process_uid = pack.get_uid();
            //LOG(INFO) << "SUBTYPE_RPC_SERVER m_process_uid = " << m_process_uid;
            rpc_event_server(pack, source, session);
            m_process_uid = 0;
        }
        else if(sub_type == SUBTYPE_PLAIN_TEXT){
            //LOG(INFO) << "SUBTYPE_PLAIN_TEXT";
			// todo  add by dik
            /*string stream(data, size);
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
            m_process_uid = 0;*/
        }
        
        break;
    }

    case PTYPE_CLIENT:
    {
        //LOG(INFO) << "Service::service_poll PTYPE_CLIENT";
		// todo add by dik
        // message(data, size, source);
        break;
    }

    case PTYPE_RESPONSE: // Timer and RPC RESPONSE 可以根据source来判断是rpc还是timer
    {
        LOG(INFO) << "PTYPE_RESPONSE source = " << source << " m_parent =  " << m_parent;
        if (source == 0) {
            timer_timeout(session);
        }
        else {
            InPack pack;
			if (!pack.inner_reset(data, size))
			{
				LOG(ERROR) << "dispatch message pack error";
				return;
			}
			uint32_t sub_type = pack.get_sub_type();
            if (sub_type == SUBTYPE_RPC_CLIENT) {
                m_process_uid = pack.get_uid();
                //LOG(INFO) << "SUBTYPE_RPC_CLIENT m_process_uid = " << m_process_uid;
                rpc_event_client(pack);
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



void Service::proto(InPack& pack, uint32_t source)
{
    m_process_uid = pack.get_uid();

    //LOG(INFO) << "proto m_process_uid = " << m_process_uid;

    DispatcherStatus status = m_dsp.dispatch_message(pack, source);
    if (status == DISPATCHER_PACK_ERROR || status == DISPATCHER_PB_ERROR)
    {
        LOG(ERROR) << "inner proto to service error";
    }

    if (status == DISPATCHER_CALLBACK_ERROR)
    {
        LOG(WARNING) << "dispatch error. service name:" << m_service_name << " pb name:" << pack.get_pbname();
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
