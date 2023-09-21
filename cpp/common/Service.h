#ifndef __SERVICE_H__
#define __SERVICE_H__
#include <map>
#include <set>
#include <stdint.h>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "Pack.h"
#include "Dispatcher.h"
#include "Timer.h"
#include "RPC.h"
#include "common.h"
#include "skynet_wrapper.h"


class skynet_context;

typedef boost::function<bool(const string&)> InitFunc;

class RegisterInitFunc
{
public:
	void register_init_func(InitFunc func)
	{
		if (!m_func.empty())
		{
			LOG(ERROR) << "error";
		}
		m_func = func;
	}
	bool call(const string& parm)
	{
		if (m_func.empty())
		{
			LOG(ERROR) << "error";
			return false;
		}

		return m_func(parm);
	}

private:
	InitFunc m_func;
};

//Service子类需要做的事情：
//1、要调用service_register_init_func，注册自己的初始化函数
//2、service_name注册内部服务名称。
class Service : public Timer, public RpcClient, public RpcServer
{
public:
	Service();
	virtual ~Service();
	bool service_init(skynet_context* ctx, const void* parm, int len);

	virtual void service_poll(const char* data, uint32_t size, uint32_t source, int session, int type);

	//内部消息回调
	void proto(InPack& pack, uint32_t source);

	virtual void text_message(const void * msg, size_t sz, uint32_t source, int session){}

	void text_response(const string& rsp);

	virtual void message(const char* data, int32_t size, uint32_t agent){}

	void service_callback(const string& name, const DispatcherT<uint32_t>::Callback& func);

	const std::string& service_name()
	{
		return m_service_name;
	}
	void service_depend_on(const string& name);  //设置service的依赖

	// void proto_service_name_brc(Message* data, uint32_t handle);

	virtual void name_got_event(const string& name){}//得到name时回调

	void set_parent(uint32_t handle)
	{
		m_parent = handle;
	}

	uint32_t get_parent()
	{
		return m_parent;
	}

	uint32_t new_skynet_service(const string& name, const string& param);

	//服务依赖完成
	virtual void service_start(){}

	//主动关闭服务，这个函数调用后，不能再执行其他语句
	void service_over();

	bool is_service_ok()
	{
		return m_service_started;
	}

	void service_register_init_func(InitFunc func)
	{
		m_init_func.register_init_func(func);
	}

	skynet_context* ctx() const
	{
		return m_service_context.get_skynet_context();
	}

	int32_t get_service_roomid()
	{
		return m_service_roomid;
	}

	void set_service_roomid(int32_t service_roomid)
	{
		m_service_roomid = service_roomid;
	}

private:
	void add_child(uint32_t handle)
	{
		m_children.insert(handle);
	}

	bool del_child(uint32_t handle)
	{
		return m_children.erase(handle) > 0;
	}

	bool is_depend_ok();

protected:
	ServiceContext  m_service_context;
	DispatcherT<uint32_t> m_dsp;

    std::string m_service_name; //给其他service提供依赖。

	std::map<std::string, uint32_t> m_named_service;

	uint32_t m_source;
	int m_session;
	int m_msg_type;
	string m_msg_data;
	int m_service_roomid;

	uint32_t m_cservice_proxy; // add by dik
private:

	bool m_service_started;

	RegisterInitFunc m_init_func;
public:
	skynet_context* m_ctx;
	uint32_t m_parent; //父服务
	std::set<uint32_t> m_children; //子服务

	int64_t m_process_uid; //当前正在处理的uid, 0 不针对任何uid
};

#endif

