#ifndef __DISPATCHER_H__
#define __DISPATCHER_H__
#include "Pack.h"

#include <google/protobuf/message.h>

#include <map>

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "ServiceContext.h"

#ifndef NDEBUG
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_base_of.hpp>
#endif

#include <assert.h>
#include "../log/Log.h"

extern "C"
{
#include "skynet_env.h"
}

typedef boost::shared_ptr<google::protobuf::Message> MessagePtr;


class Callback : boost::noncopyable
{
public:
	virtual ~Callback() {};
	virtual void onMessage(const MessagePtr& message) const = 0;
};

template <typename T>
class CallbackT : public Callback
{
#ifndef NDEBUG
	BOOST_STATIC_ASSERT((boost::is_base_of<google::protobuf::Message, T>::value));
#endif
public:
	typedef boost::function<void(const boost::shared_ptr<T>& message)> ProtobufMessageTCallback;

	CallbackT(const ProtobufMessageTCallback& callback)
			: callback_(callback)
	{
	}

	virtual void onMessage(const MessagePtr& message) const
	{
		boost::shared_ptr<T> concrete = down_pointer_cast<T>(message);
		assert(concrete != NULL);
		callback_(concrete);
	}

private:
	ProtobufMessageTCallback callback_;
};

class ProtobufDispatcher
{
public:
	typedef boost::function<void(const MessagePtr& message)> ProtobufMessageCallback;

	explicit ProtobufDispatcher(const ProtobufMessageCallback& defaultCb)
			: defaultCallback_(defaultCb)
	{
	}

	void onProtobufMessage(const MessagePtr& message) const
	{
		CallbackMap::const_iterator it = callbacks_.find(message->GetDescriptor());
		if (it != callbacks_.end())
		{
			it->second->onMessage(message);
		}
		else
		{
			defaultCallback_(message);
		}
	}

	template<typename T>
	void registerMessageCallback(const typename CallbackT<T>::ProtobufMessageTCallback& callback)
	{
		boost::shared_ptr<CallbackT<T> > pd(new CallbackT<T>(callback));
		callbacks_[T::descriptor()] = pd;
	}

private:
	typedef std::map<const google::protobuf::Descriptor*, boost::shared_ptr<Callback> > CallbackMap;

	CallbackMap callbacks_;
	ProtobufMessageCallback defaultCallback_;
};


enum DispatcherStatus
{
	DISPATCHER_SUCCUSS = 0,
	DISPATCHER_PACK_ERROR = 1,  //包格式错误
	DISPATCHER_PB_ERROR = 2,  //反序列化失败
	DISPATCHER_CALLBACK_ERROR = 3, //找不到pack name
	DISPATCHER_FORBIDDEN = 4,
};



template<typename T>
class DispatcherT final
{
public:
	DispatcherT(ServiceContext* service_ctx)
			: m_service_context(service_ctx)
	{
	}
	~DispatcherT()
	{
	}
	DispatcherT(const DispatcherT<T>&) = delete;
	DispatcherT<T>& operator= (const DispatcherT<T>&) = delete;

	typedef boost::function<void(Message*, T)> Callback;
	typedef std::map<std::string, Callback> CallbackMap;

	void register_callback(const string& name, const Callback& func)
	{
		// 用于存储protobuf name和回调函数
		m_callback[name] = func;
	}

	DispatcherStatus dispatch_message(const char* data, uint32_t size, T user)
	{
		// 分发内部协议
		return dispatch_(data, size, user, true);
	}

	//分发客户端消息
	DispatcherStatus dispatch_client_message(const char* data, uint32_t size, T user)
	{
		// 分发外部协议
		LOG(INFO) << "dispatch client message";
		return dispatch_(data, size, user, false);
	}

	CallbackMap m_callback;

	static void onMessage(const MessagePtr& message)
	{
		LOG(INFO) << "Disppatcher onMessage message = " << message->DebugString();
	}

private:
	DispatcherStatus dispatch_(const char* data, uint32_t size, T user, bool inner)
	{
		InPack pack;
		bool b = false;
		if (inner)
		{
			// 解析内部协议 uid
			//LOG(INFO) << "parse inner message";
			b = pack.inner_reset(data, size);
		}
		else
		{
			// 解析外部协议 不带uid
			LOG(INFO) << "parse client message";
			b = pack.reset(data, size);

			//ProtobufCodec codec(onMessage);
			//codec.onMessage(data, size);

		}
		if (!b)
		{
			log_error("dispatch message pack error");
			return DISPATCHER_PACK_ERROR;
		}
//		LOG(INFO) << "dispatch m_type_name = " << pack.m_type_name;


		typename CallbackMap::iterator it = m_callback.find(pack.m_type_name);
		if (it != m_callback.end())
		{

			Message* msg = pack.create_message();
			if (msg == NULL)
			{
				log_error("dispatch message pb error type:%s", pack.m_type_name.c_str());
				return DISPATCHER_PB_ERROR;
			}

			if (m_service_context->profile())
			{
				it->second(msg, user);
			}
			else
			{
				it->second(msg, user);  // user在watchdog中是fd, 在其他的服务中是User的实例
			}

			delete msg;
			return DISPATCHER_SUCCUSS;
		}
//		LOG(WARNING) << "proto type no find. type:" << pack.m_type_name;
		return DISPATCHER_CALLBACK_ERROR;
	}

private:
	ServiceContext* m_service_context;
};

#endif