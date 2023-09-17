#include "common.h"
#include "RPC.h"
#include "../log/Log.h"

void RPC_NULL_FUNC(Message* msg)
{}

RpcClient::RpcClient()
{
	//m_r_idx = 1;
	m_service_context = nullptr;
}

void RpcClient::rpc_init_client(ServiceContext* service_ctx)
{
	m_service_context = service_ctx;
}

int RpcClient::rpc_call(int32_t dest, const google::protobuf::Message& msg, const RPCCallBack& func, int64_t uid)
{
	if (m_service_context == nullptr)
	{
		LOG(ERROR) << "m_service_context is nullptr";
		return 0;
	}
	char* data;
	uint32_t size;
	//int session = m_r_idx++;
	int session = skynet_context_newsession(m_service_context->get_skynet_context());
	LOG(INFO) << "rpc_call skynet_context_newsession session = " << session << " msgname = " << msg.GetTypeName() << " uid = " << uid;
	serialize_imsg_type(msg, data, size, uid, SUBTYPE_RPC_SERVER, 0);
	//skynet_send_noleak(m_rpc_ctx, 0, dest, PTYPE_RPC_SERVER | PTYPE_TAG_DONTCOPY, session, data, size);
	skynet_send_noleak(m_service_context->get_skynet_context(), 0, dest, PTYPE_TEXT | PTYPE_TAG_DONTCOPY, session, data, size);
	//log_debug("rpc call. size:%d %s", size, msg.GetTypeName().c_str());
	m_rpc[session] = func;
	return session;
}

int RpcClient::rpc_call_proxy(int32_t dest, const google::protobuf::Message& msg, const RPCCallBack& func, int64_t uid, const string& remote_node, const string& remote_service, const string& source_node, const string& source_service)
{
	if (m_service_context == nullptr)
	{
		LOG(ERROR) << "m_service_context is nullptr";
		return 0;
	}
	char* data;
	uint32_t size;
	int session = skynet_context_newsession(m_service_context->get_skynet_context());
	int32_t roomid = 0;
	// LOG(INFO) << "rpc_call_proxy session:" << session;
	serialize_imsg_proxy(msg, data, size, uid, SUBTYPE_RPC_SERVER, roomid, session, remote_node, remote_service, source_node, source_service);
	skynet_send_noleak(m_service_context->get_skynet_context(), 0, dest, PTYPE_TEXT | PTYPE_TAG_DONTCOPY, 0, data, size);
	m_rpc[session] = func;
	return session;
}

void RpcClient::rpc_cancel(int& id)
{
	m_rpc.erase(id);
	id = 0;
}

void RpcClient::rpc_event_client(const char* data, uint32_t size, uint32_t source, int session)
{
	LOG(INFO) << "rpc_event_client source = " << source << " session = " << session;
	std::map<int, RPCCallBack>::iterator it = m_rpc.find(session);
	if (it == m_rpc.end())
		return;
	if (it->second == RPC_NULL_FUNC)
		return;

	InPack pack;
	if (!pack.inner_reset(data, size))
	{
		log_error("dispatch message pack error");
		return;
	}
	Message* msg = pack.create_message();
	if (msg == NULL)
	{
		LOG(ERROR) << "rpc client message pb error";
		return;
	}

	LOG(INFO) << "rpc_event_client call func for msg = " << msg->GetTypeName();
	if (m_service_context->profile())
	{
		it->second(msg);
	}
	else
	{
		it->second(msg);
	}


	delete msg;
	m_rpc.erase(session);
}

void RpcClient::rpc_event_client_proxy(const char* data, uint32_t size)
{
	InPackCluser pack;
	if (!pack.inner_reset(data, size))
	{
		log_error("dispatch message pack error");
		return;
	}

	int session = pack.get_session();
	std::map<int, RPCCallBack>::iterator it = m_rpc.find(session);
	if (it == m_rpc.end())
	{
		return;
	}
	if (it->second == RPC_NULL_FUNC)
		return;

	Message* msg = pack.create_message();
	if (msg == NULL)
	{
		LOG(ERROR) << "rpc client message pb error";
		return;
	}

	LOG(INFO) << "rpc_event_client_proxy call func for msg = " << msg->GetTypeName();
	if (m_service_context->profile())
	{
		it->second(msg);
	}
	else
	{
		it->second(msg);
	}

	delete msg;
	m_rpc.erase(session);
}


RpcServer::RpcServer()
{
	m_source = 0;
	m_session = 0;
	m_uid = 0;

	m_service_context = nullptr;
}

DispatcherStatus RpcServer::rpc_event_server(const char* data, uint32_t size, uint32_t source, int session)
{
	if (m_service_context == nullptr)
	{
		LOG(ERROR) << "m_service_context is nullptr";
		return DISPATCHER_FORBIDDEN;
	}
	InPack pack;
	if (!pack.inner_reset(data, size))
	{
		log_error("dispatch message pack error");
		return DISPATCHER_PACK_ERROR;
	}
	LOG(INFO) << "rpc_event_server session = " << session << " pack.m_type_name = " << pack.m_type_name;
	CallbackMap::iterator it = m_callback.find(pack.m_type_name);
	if (it != m_callback.end())
	{
		Message* msg = pack.create_message(); 
		if (msg == NULL)
		{
			log_error("dispatch message pb error");
			return DISPATCHER_PB_ERROR;
		}
		m_source = source;
		m_session = session;
		m_uid = pack.uid();
		LOG(INFO) << "m_source = " << m_source << " m_session = " << m_session << " m_uid = " << m_uid;

		Message* rsp;
		if (m_service_context->profile())
		{
			rsp = it->second(msg);
		}
		else
		{
			rsp = it->second(msg);
		}


		if (rsp != NULL) {
			LOG(INFO) << "rpc_event_server call response typename = " << rsp->GetTypeName() << " uid =  " << pack.uid();
			response(rsp, source, session, pack.uid());
		}
			
		delete msg;
		return DISPATCHER_SUCCUSS;
	}
	log_error("rpc dispatch message callback error, type:%s", pack.m_type_name.c_str());
	return DISPATCHER_CALLBACK_ERROR;
}

RSPFunction RpcServer::get_response()
{
	return boost::bind(&RpcServer::response, this, _1, m_source, m_session, m_uid);
}

void RpcServer::rpc_init_server(ServiceContext* service_ctx)
{
	m_service_context = service_ctx;
}

void RpcServer::response(Message* msg, uint32_t dest, int session, int64_t uid)
{
	char* data;
	uint32_t size;
	serialize_imsg_type(*msg, data, size, uid, SUBTYPE_RPC_CLIENT, 0);
	//skynet_send_noleak(m_rpc_ctx, 0, dest, PTYPE_RPC_CLIENT | PTYPE_TAG_DONTCOPY, session, data, size);
	skynet_send_noleak(m_service_context->get_skynet_context(), 0, dest, PTYPE_RESPONSE | PTYPE_TAG_DONTCOPY, session, data, size);
	log_debug("rpc call response. size:%d %s", size, msg->GetTypeName().c_str());
	delete msg;
}
