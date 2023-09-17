
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include "Pack.h"
#include "../log/Log.h"
extern "C"
{
#include "skynet_socket.h"
#include "skynet_server.h"
#include "skynet.h"
#include "skynet_timer.h"
#include "skynet_handle.h"
#include "skynet_env.h"
}
using namespace std;


InPack::InPack()
{
	m_pb_data = NULL; // protobuf数据
}

bool InPack::inner_reset(const char* cdata, uint32_t size)
{
	// 解析消息头部
	m_remote_node_len = get_uint16(cdata, size);
	m_remote_service_len = get_uint16(cdata, size);
	m_source_node_len = get_uint16(cdata, size);
	m_source_service_len = get_uint16(cdata, size);

	// LOG(INFO) << "inner_reset " 
	// 	<< m_remote_node_len << " "
	// 	<< m_remote_service_len << " "
	// 	<< m_source_node_len << " "
	// 	<< m_source_node_len;

	if (m_remote_node_len > 0) {
		get_string(cdata, size, m_remote_node_name, m_remote_node_len);
	}
	if (m_remote_service_len) {
		get_string(cdata, size, m_remote_service_name, m_remote_service_len);
	}
	if (m_source_node_len > 0) {
		get_string(cdata, size, m_source_node_name, m_source_node_len);
	}
	if (m_source_service_len > 0) {
		get_string(cdata, size, m_source_service_name, m_source_service_len);
	}

	m_sub_type = get_uint32(cdata, size);
	m_uid = get_int64(cdata, size);
	m_pbname_len = get_uint16(cdata, size);
	get_string(cdata, size, m_pbname, m_pbname_len);
	m_roomid = get_uint32(cdata, size);
	m_session = (uint64_t)get_int64(cdata, size);

	// LOG(INFO) << "inner_reset " 
	// 	<< m_sub_type << " "
	// 	<< m_pbname;

	// 解析消息体
	if (size < 0)
	{
		return false;
	}

	m_pb_data = cdata; // 解析pb数据
	m_pb_data_len = size; // 解析pb数据长度

	return true;
}

Message* InPack::create_message()
{
	Message* message = NULL;
	const Descriptor* descriptor = DescriptorPool::generated_pool()->FindMessageTypeByName(m_pbname);
	if (descriptor)
	{
		const Message* prototype = MessageFactory::generated_factory()->GetPrototype(descriptor);
		if (prototype)
		{
			message = prototype->New();
			if (!message->ParseFromArray(m_pb_data, m_pb_data_len))
			{
				delete message;
				message = NULL;
				LOG(ERROR) << "pb parse error type:" << m_pbname.c_str();
			}
		}
	}
	return message;
}

OutPack::OutPack(const Message& msg, const std::string& remote_node, const std::string& remote_service, const string& source_node, const string& source_service)
{
	m_remote_node_name = remote_node;
	m_remote_service_name = remote_service;
	m_source_node_name = source_node;
	m_source_service_name = source_service;
	reset(msg);
}

bool OutPack::reset(const Message& msg)
{
	if (!msg.SerializeToString(&m_pb_data)) // 把protobuf消息msg序列化为string并保存到m_pb_data中
	{
		log_error("serialize error\n");
		return false;
	}
	m_type_name = msg.GetTypeName(); // 获取协议名称
	return true;
}

void OutPack::new_innerpack_proxy(char* &result, uint32_t& size, uint64_t uid, uint32_t type, int32_t roomid, uint64_t session)
{
	// 【远程节点名长度】【远程服务名长度】【源节点名长度】【源服务名长度】【远程节点名】【远程服务名】【源节点名】【源服务名】【sub_type】【uid】【协议名称长度】【协议名称】【roomid】【session】【数据】
	// LOG(INFO) << "new_innerpack_proxy uid:" << uid << " type:" << type << " roomid:" << roomid << " session:" << session;
	int remote_node_len = m_remote_node_name.size();
	int remote_service_len = m_remote_service_name.size();
	int source_node_len = m_source_node_name.size();
	int source_service_len = m_source_service_name.size();
	int pbname_len = m_type_name.size();
	int pbdata_len = m_pb_data.size();
	int data_len = kNameLen + kNameLen + kNameLen + kNameLen + remote_node_len + remote_service_len + source_node_len + source_service_len + kSubTypeLen + kUidLen + kTypeNameLen + pbname_len + kRoomidLen + kSessionLen + pbdata_len;

	char* data = (char*)skynet_malloc(data_len);
	result = data;
	size = data_len;

	memset(data, 0, data_len);

	// remote node name len
	char* tmp_data = data;
	uint16_t node_len = htons(remote_node_len);
	memcpy(tmp_data, &node_len, kNameLen); 

	// remote service name len
	tmp_data = data + kNameLen;
	uint16_t service_len = htons(remote_service_len);
	memcpy(tmp_data, &service_len, kNameLen);

	// source node name len
	tmp_data = data + kNameLen + kNameLen;
	uint16_t htons_source_node_len = htons(source_node_len);
	memcpy(tmp_data, &htons_source_node_len, kNameLen);

	// source service name len
	tmp_data = data + kNameLen + kNameLen + kNameLen;
	uint16_t htons_source_service_len = htons(source_service_len);
	memcpy(tmp_data, &htons_source_service_len, kNameLen);

	// remote node name
	tmp_data = data + kNameLen + kNameLen + kNameLen + kNameLen;
	memcpy(tmp_data, m_remote_node_name.c_str(), remote_node_len);

	// remote service name
	tmp_data = data + kNameLen + kNameLen + kNameLen + kNameLen + remote_node_len;
	memcpy(tmp_data, m_remote_service_name.c_str(), remote_service_len);

	// source node name
	tmp_data = data + kNameLen + kNameLen + kNameLen + kNameLen + remote_node_len + remote_service_len;
	memcpy(tmp_data, m_source_node_name.c_str(), source_node_len);

	// source service name
	tmp_data = data + kNameLen + kNameLen + kNameLen + kNameLen + remote_node_len + remote_service_len + source_node_len;
	memcpy(tmp_data, m_source_service_name.c_str(), source_service_len);

	// sub_type
	tmp_data = data + kNameLen + kNameLen + kNameLen + kNameLen + remote_node_len + remote_service_len + source_node_len + source_service_len;
	uint32_t htonl_sub_type = htonl(type); 
	memcpy(tmp_data, &htonl_sub_type, sizeof(uint32_t));

	// uid
	tmp_data = data + kNameLen + kNameLen + kNameLen + kNameLen + remote_node_len + remote_service_len + source_node_len + source_service_len + kSubTypeLen;
	uint64_t hton64_uid = hton64(&uid);
	memcpy(tmp_data, &hton64_uid, kUidLen);

	// namelen
	tmp_data = data + kNameLen + kNameLen + kNameLen + kNameLen + remote_node_len + remote_service_len + source_node_len + source_service_len + kSubTypeLen + kUidLen;
	uint16_t htonsTypeName = htons(pbname_len);
	memcpy(tmp_data, &htonsTypeName, kTypeNameLen);

	// pbname
	tmp_data = data + kNameLen + kNameLen + kNameLen + kNameLen + remote_node_len + remote_service_len + source_node_len + source_service_len + kSubTypeLen + kUidLen + kTypeNameLen;
	memcpy(tmp_data, m_type_name.c_str(), pbname_len);

	// roomid
	tmp_data = data + kNameLen + kNameLen + kNameLen + kNameLen + remote_node_len + remote_service_len + source_node_len + source_service_len + kSubTypeLen + kUidLen + kTypeNameLen + pbname_len;
	int32_t head_roomid = htonl(roomid);
	memcpy(tmp_data, &head_roomid, kRoomidLen);

	// session
	tmp_data = data + kNameLen + kNameLen + kNameLen + kNameLen + remote_node_len + remote_service_len + source_node_len + source_service_len + kSubTypeLen + kUidLen + kTypeNameLen + pbname_len + kRoomidLen;
	uint64_t hton64_session = hton64(&session);
	memcpy(tmp_data, &hton64_session, kSessionLen);

	// pbdata
	tmp_data = data + kNameLen + kNameLen + kNameLen + kNameLen + remote_node_len + remote_service_len + source_node_len + source_service_len + kSubTypeLen + kUidLen + kTypeNameLen + pbname_len + kRoomidLen + kSessionLen;
	memcpy(tmp_data, m_pb_data.c_str(), pbdata_len);
}

/////////////

const string TextParm::nullstring;

void TextParm::deserilize(const char* stream, size_t len)
{
	char key[10000];
	char value[10000];
	int klen, vlen;
	if (len > 10000)
		return;
	map<string, string> temp_map;
	size_t i = 0;
	while (i < len)
	{
		klen = 0;
		if (stream[i] == ' ')
		{
			++i;
			continue;
		}
		while (i < len && stream[i] != '=')
			key[klen++] = stream[i++];
		key[klen] = 0;
		++i;
		vlen = 0;
		while (i < len && stream[i] != ' ')
			value[vlen++] = stream[i++];
		value[vlen] = 0;
		++i;
		temp_map[key] = value;
	}
	m_data.swap(temp_map);
	m_stream.assign(stream);
}

const char* TextParm::serilize()
{
	map<string, string>::const_iterator it = m_data.begin();
	m_stream.clear();
	while (it != m_data.end())
	{
		m_stream += it->first;
		m_stream.push_back('=');
		m_stream += it->second;
		m_stream.push_back(' ');
		++it;
	}
	return m_stream.c_str();
}
////////////////

void serialize_imsg_proxy(const Message& msg, char*& result, uint32_t& size, uint64_t uid, uint32_t sub_type, int32_t roomid, uint64_t session, const string& remote_node, const string& remote_service, const string& source_node, const string& source_service)
{
	OutPack opack(msg, remote_node, remote_service, source_node, source_service);
	opack.new_innerpack_proxy(result, size, uid, sub_type, roomid, session);
}


