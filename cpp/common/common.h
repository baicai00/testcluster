#ifndef _COMMON_H_
#define _COMMON_H_


#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/operators.hpp>
#include <boost/implicit_cast.hpp>

#define SUBTYPE_PROTOBUF 16  // PTYPE_PROTO
#define SUBTYPE_PLAIN_TEXT 19

#define SUBTYPE_RPC_CLIENT 17 // PTYPE_RPC_CLIENT
#define SUBTYPE_RPC_SERVER 18 // PTYPE_RPC_SERVER
#define SUBTYPE_AGENT 20 // 给客户端下行消息
#define SUBTYPE_SYSTEM 21 // 服务内部系统消息

#define PTYPE_AGENT 21


// Adapted from google-protobuf stubs/common.h
// see License in muduo/base/Types.h
template<typename To, typename From>
inline boost::shared_ptr<To> down_pointer_cast(const boost::shared_ptr<From>& f)
{

#ifndef NDEBUG
	assert(f == NULL || dynamic_cast<To*>(get_pointer(f)) != NULL);
#endif
	return boost::static_pointer_cast<To>(f);
}

inline std::tuple<std::string, std::string> divide_string(const std::string& source, char gap)
{
	auto idx = source.find(gap);
	std::string fir;
	std::string sec;
	if (idx != std::string::npos)
	{
		fir = source.substr(0, idx);
		sec = source.substr(idx + 1);
	}
	else
	{
		fir = source;
	}
	return make_tuple(fir, sec);
}

inline int strlen_1(const char* str)
{
	if (str == NULL)
		return 0;
	return strlen(str);
}

#endif