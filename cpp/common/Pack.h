#ifndef __PACK_H__
#define __PACK_H__
#include <google/protobuf/message.h>
#include <google/protobuf/descriptor.h>
#include <stdint.h>
#include <string>
#include <stdio.h>
#include <map>
#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include "common.h"
// #include "../pb/inner.pb.h"
// #include "../pb/kktp1.pb.h"
// #include "../pb/kktp2.pb.h"

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

/*
wire format
消息格式: 与客户端通信
uint32_t dataLen
uint16_t nameLen
char typeName[namelen]
uint32_t roomid
char protobufData[dataLen - namelen]

消息格式: 与其他服务进行通信
uint64_t uid
uint16_t namelen
char typeName[namelen]
uint32_t roomid
char protobufData[len - uid - namelen]
*/


inline uint64_t get_uid_from_stream(const void* data)
{
	if (data == NULL)
		return 0;
	uint64_t uid = *(uint64_t *)data;
	//LOG(INFO) << "get_uid_from_stream uid = " << uid;
	uint64_t ntoh64_uid = ntoh64(&uid);
	//LOG(INFO) << "get_uid_from_stream ntoh64_uid = " << ntoh64_uid;
	return ntoh64_uid;
}

//char* to message
// 解码器
// 把从网络端接受的数据反序列化成protobuf消息
class InPack
{
public:
	InPack();
	bool reset(const char* data, uint32_t size = 0);
    bool out_reset(const char* data, uint32_t size);
	//反序列号带uid的流 用于服务间的rpc或者单
	bool inner_reset(const char* cdata, uint32_t size = 0);

	uint64_t uid()
	{
		return m_uid;
	}

	//得到pb包
	Message* create_message();

	void test();
	std::string m_type_name; // 协议名称
	int32_t m_roomid; //房间id

protected:
	InPack(const char* data, uint32_t size = 0);

	const char* m_data; // buffer
	uint32_t m_data_len; // buffer lenth
	const char* m_pb_data;
	uint32_t m_pb_data_len;

	int64_t m_uid; // 发送协议的uid

    //解出m_type_name
	bool decode_stream(const char* data, uint32_t size);

	const static int kMessageLen = 4; // 消息的总长度 用4个byte来存储消息的总长度 PACK_HEAD
	const static int kTypeNameLen = 2; // 协议名称的长度 TYPE_HEAD
	const static int kMaxTypeNameLen = 50; // 协议名称的最大长度 MAX_TYPE_SIZE
	const static int kRoomidLen = 4; // roomid 
	const static int kMaxMessageLen = 0x1000000; // 64 * 1024 * 1024 MAX_PACK_SIZE
};

class InPackCluser : public InPack
{
public:
	bool inner_reset(const char* cdata, uint32_t size = 0);
	inline int get_session()
	{
		return m_session;
	}

private:
	uint64_t m_session;
};

//message to char*
// 把protobuf消息序列化成char*用于发送给网络端(需要添加包头)
// 发送给内部的服务 uid + 
// 发送给外部的服务 总长度 +
class OutPack
{
public:
	OutPack() {}
	OutPack(const Message& msg)
	{
		reset(msg);
	}
	bool reset(const Message& msg);
	void new_outpack(char* &result, uint32_t& size, uint32_t sub_type, const int32_t& roomid); // char* &result:指向char*的引用
	void new_innerpack_type(char* &result, uint32_t& size, uint64_t uid, uint32_t type, const int32_t& roomid); //用于rpc
	void new_innerpack(char* &result, uint32_t& size, uint64_t uid, const int32_t& roomid); //用于rpc
	void test();

	std::string m_pb_data; // pb数据的字符串
	std::string m_type_name; // pb协议的名称

	const static int kMessageLen = 4; // 消息的总长度 用4个byte来存储消息的总长度 PACK_HEAD
	const static int kTypeNameLen = 2; // 协议名称的长度 TYPE_HEAD
	const static int kMaxTypeNameLen = 50; // 协议名称的最大长度 MAX_TYPE_SIZE
	const static int kRoomidLen = 4; // roomid 
	const static int kMaxMessageLen = 0x1000000; // 64 * 1024 * 1024 MAX_PACK_SIZE
};

class OutPackProxy : public OutPack
{
public:
	OutPackProxy(const Message& msg, const std::string& remote_node, const std::string& remote_service, const string& source_node, const string& source_service);
	void new_innerpack_proxy(char* &result, uint32_t& size, uint64_t uid, uint32_t type, int32_t roomid, uint64_t session);

	std::string m_remote_node_name;
	std::string m_remote_service_name;
	std::string m_source_node_name;
	std::string m_source_service_name;

	const static int kUidLen = 8;
	const static int kSubTypeLen = 4;
	const static int kNameLen = 2; // 远程节点与服务名称的长度
	const static int kSessionLen = 8;
};

// 外部的协议，需要和客户端通信
void serialize_msg(const Message& msg, char* &result, uint32_t& size, uint32_t type, const int32_t& roomid);
// 内部的协议，不和客户端通信
void serialize_imsg_type(const Message& msg, char* &result, uint32_t& size, uint64_t uid, uint32_t type, const int32_t& romid);  //不加包头

void serialize_imsg(const Message& msg, char* &result, uint32_t& size, uint64_t uid, const int32_t& roomid);

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
