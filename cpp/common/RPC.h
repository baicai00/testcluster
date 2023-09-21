#ifndef __RPC_H__
#define __RPC_H__
#include <map>
#include <stdint.h>
#include "ServiceContext.h"
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "Pack.h"
#include "Dispatcher.h"
#include "skynet_wrapper.h"


typedef boost::function<void(Message* msg)> RPCCallBack;

typedef boost::function<void(Message* msg)> RSPFunction;

void RPC_NULL_FUNC(Message* msg);

class skynet_context;
class RpcClient
{
public:
	RpcClient();
	void rpc_init_client(ServiceContext*);
	int rpc_call(int32_t dest, const google::protobuf::Message& msg, const RPCCallBack& func, int64_t uid, const string& remote_node, const string& remote_service, const string& source_node, const string& source_service);
	void rpc_cancel(int& id);
	void rpc_event_client(InPack& pack);
private:
	//int m_r_idx; //session id
	std::map<int, RPCCallBack> m_rpc; //
	ServiceContext* m_service_context;
};


class RpcServer
{
public:
	typedef boost::function<Message*(Message*)> Callback;
	typedef std::map<std::string, Callback> CallbackMap;

	RpcServer();
	void rpc_init_server(ServiceContext*, uint32_t);
	void rpc_regester(const string& name, const Callback& func)
	{
		m_callback[name] = func;
	}
	DispatcherStatus rpc_event_server(InPack& pack, uint32_t source, int session);

	// RSPFunction get_response(); //RPC返回值

	CallbackMap m_callback;

private:

	void response(Message* msg, int session, int64_t uid, const std::string& remote_node, const std::string& remote_service, const std::string& source_node, const std::string& source_service);


	ServiceContext* m_service_context;
	uint32_t m_cservice_proxy;

	uint32_t m_source;
	int m_session;
	int64_t m_uid;
};

#endif
