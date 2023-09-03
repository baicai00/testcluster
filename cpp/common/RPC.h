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
	int rpc_call(int32_t dest, const google::protobuf::Message& msg, const RPCCallBack& func, int64_t uid);
	void rpc_cancel(int& id);
	void rpc_event_client(const char* data, uint32_t size, uint32_t source, int session);
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
	void rpc_init_server(ServiceContext*);
	void rpc_regester(const string& name, const Callback& func)
	{
		m_callback[name] = func;
	}
	DispatcherStatus rpc_event_server(const char* data, uint32_t size, uint32_t source, int session);

	RSPFunction get_response(); //RPC返回值

	CallbackMap m_callback;

private:

	void response(Message* msg, uint32_t dest, int session, int64_t uid);


	ServiceContext* m_service_context;

	uint32_t m_source;
	int m_session;
	int64_t m_uid;
};

#endif
