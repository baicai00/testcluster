#ifndef __PACK_H__
#define __PACK_H__
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <stdint.h>
#include <string>
#include <stdio.h>
#include <arpa/inet.h>
#include <map>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include "common.h"

extern "C"
{
#include "skynet_socket.h"
#include "skynet_server.h"
#include "skynet.h"
#include "skynet_timer.h"
#include "skynet_handle.h"
#include "skynet_env.h"
}
using namespace google;
using namespace protobuf;
using namespace std;

/*
[[
	【远程节点名长度】  占2字节
	【远程服务名长度】  占2字节
	【源节点名长度】    占2字节
	【源服务名长度】    占2字节
	【远程节点名】
	【远程服务名】
	【源节点名】
	【源服务名】
	【sub_type】      占4字节
	【uid】           占8字节
	【协议名称长度】    占2字节
	【协议名称】
	【roomid】        占4字节
	【session】       占8字节
	【数据】
]]
*/

inline uint64_t ntoh64(uint64_t *input)
{
	uint64_t rval;
	uint8_t *data = (uint8_t *)&rval;

	data[0] = *input >> 56;
	data[1] = *input >> 48;
	data[2] = *input >> 40;
	data[3] = *input >> 32;
	data[4] = *input >> 24;
	data[5] = *input >> 16;
	data[6] = *input >> 8;
	data[7] = *input >> 0;

	return rval;
}

inline uint64_t hton64(uint64_t *input)
{
	return (ntoh64(input));
}

inline uint16_t get_uint16(const char*& data, uint32_t& size)
{
	uint16_t tmp = 0;
	int len = sizeof(uint16_t);
	memcpy(&tmp, data, len);
	data += len;
	size -= len;
	return ntohs(tmp);
}

inline uint32_t get_uint32(const char*& data, uint32_t& size)
{
	uint32_t tmp = 0;
	int len = sizeof(uint32_t);
	memcpy(&tmp, data, len);
	data += len;
	size -= len;
	return ntohl(tmp);
}

inline int64_t get_int64(const char*& data, uint32_t& size)
{
	uint64_t tmp = 0;
	int len = sizeof(uint64_t);
	memcpy(&tmp, data, len);
	data += len;
	size -= len;
	return (int64_t)ntoh64(&tmp);
}

inline void get_string(const char*& data, uint32_t& size, std::string& strOut, int strSize)
{
	strOut.assign(data, strSize);
	data += strSize;
	size -= strSize;
}

//char* to message
// 解码器
// 把从网络端接受的数据反序列化成protobuf消息
class InPack
{
public:
	InPack();
	bool inner_reset(const char* cdata, uint32_t size);

	inline uint64_t get_session()
	{
		return m_session;
	}

	inline int64_t get_uid()
	{
		return m_uid;
	}

	inline uint32_t get_sub_type()
	{
		return m_sub_type;
	}

	inline string get_pbname()
	{
		return m_pbname;
	}

	inline string get_remote_node() {
		return m_remote_node_name;
	}

	inline string get_remote_service() {
		return m_remote_service_name;
	}

	inline string get_source_node() {
		return m_source_node_name;
	}

	inline string get_source_service() {
		return m_source_service_name;
	}

	//得到pb包
	Message* create_message();

private:
	// 消息头
	uint16_t m_remote_service_len;
	uint16_t m_remote_node_len;
	uint16_t m_source_service_len;
	uint16_t m_source_node_len;
	std::string m_remote_node_name;
	std::string m_remote_service_name;
	std::string m_source_node_name;
	std::string m_source_service_name;
	uint32_t m_sub_type;
	int64_t m_uid; // 发送协议的uid
	uint16_t m_pbname_len;
	std::string m_pbname;
	uint32_t m_roomid;
	uint64_t m_session;

	// 消息体
	const char* m_pb_data;
	uint32_t m_pb_data_len;

private:
	const static int kUidLen = 8;
	const static int kSubTypeLen = 4;
	const static int kNameLen = 2; // 远程节点与服务名称的长度
	const static int kSessionLen = 8;
};

//message to char*
// 把protobuf消息序列化成char*用于发送给网络端(需要添加包头)
// 发送给内部的服务 uid + 
// 发送给外部的服务 总长度 +
class OutPack
{
public:
	OutPack() {}
	OutPack(const Message& msg, const std::string& remote_node, const std::string& remote_service, const string& source_node, const string& source_service);
	bool reset(const Message& msg);
	void new_innerpack_proxy(char* &result, uint32_t& size, uint64_t uid, uint32_t type, int32_t roomid, uint64_t session);

	std::string m_remote_node_name;
	std::string m_remote_service_name;
	std::string m_source_node_name;
	std::string m_source_service_name;

	std::string m_pb_data; // pb数据的字符串
	std::string m_type_name; // pb协议的名称

	const static int kMessageLen = 4; // 消息的总长度 用4个byte来存储消息的总长度 PACK_HEAD
	const static int kTypeNameLen = 2; // 协议名称的长度 TYPE_HEAD
	const static int kMaxTypeNameLen = 50; // 协议名称的最大长度 MAX_TYPE_SIZE
	const static int kRoomidLen = 4; // roomid 
	const static int kMaxMessageLen = 0x1000000; // 64 * 1024 * 1024 MAX_PACK_SIZE
	const static int kUidLen = 8;
	const static int kSubTypeLen = 4;
	const static int kNameLen = 2; // 远程节点与服务名称的长度
	const static int kSessionLen = 8;
};


// 用于C++服务往CServiceProxy服务发送消息
void serialize_imsg_proxy(const Message& msg, char*& result, uint32_t& size, uint64_t uid, uint32_t sub_type, int32_t roomid, uint64_t session, const string& remote_node, const string& remote_service, const string& source_node, const string& source_service);

class TextParm
{
public:
	TextParm(const char* stream)
	{
		if (stream != NULL)
			deserilize(stream, strlen(stream));
	}
	TextParm(const char* stream, int len)
	{
		if (stream != NULL)
			deserilize(stream, len);
	}
	TextParm() {}
	void deserilize(const char* stream, size_t len);
	const char* serilize();

	size_t serilize_size()
	{
		return m_stream.size();
	}
	void erase(const std::string& key)
	{
		m_data.erase(key);
	}
	void insert(const std::string& key, const std::string& value)
	{
		m_data[key] = value;
	}
	void insert_int(const std::string& key, int value)
	{
		char v[10];
		sprintf(v, "%d", value);
		m_data[key] = v;
	}
	void insert_uint(const std::string& key, uint32_t value)
	{
		char v[10];
		sprintf(v, "%u", value);
		m_data[key] = v;
	}
	const char* get(const std::string& key) const
	{
		return get_string(key).c_str();
	}
	const string& get_string(const std::string& key) const
	{
		std::map<std::string, std::string>::const_iterator it = m_data.find(key);
		if (it == m_data.end())
			return nullstring;
		return it->second;
	}
	int get_int(const std::string& key) const
	{
		return atoi(get(key));
	}

	int64_t get_int64(const std::string& key) const
	{
		return atoll(get(key));
	}

	uint32_t get_uint(const std::string& key) const
	{
		return strtoul(get(key), NULL, 10);
	}
	void print() const
	{
		std::map<std::string, std::string>::const_iterator it = m_data.begin();
		while (it != m_data.end())
		{
			printf("%s=%s\n", it->first.c_str(), it->second.c_str());
			++it;
		}
	}

	bool has(const std::string& key)
	{
		std::map<std::string, std::string>::const_iterator it = m_data.find(key);
		if (it == m_data.end())
			return false;
		return true;
	}

private:
	std::map<std::string, std::string> m_data;
	static const std::string nullstring;
	std::string m_stream;
};



inline void string_fill_space(string& str)
{
	size_t idx = 0;
	while (1)
	{
		idx = str.find("<!spc>", idx);
		if (idx == string::npos)
			break;
		str.replace(idx, 6, " ");
	}
}


#endif
