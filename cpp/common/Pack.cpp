
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
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


int32_t asInt32(const char* buf)
{
	int32_t be32 = 0;
	memcpy(&be32, buf, sizeof(be32));
	return ntohl(be32);
}

int16_t asInt16(const char* buf)
{
	int16_t be16 = 0;
	memcpy(&be16, buf, sizeof(be16));
	return ntohs(be16);
}

static bool zlib_compress(const std::string& pb_data, std::string& output)
{
    constexpr int CHUNK = 16384;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    int ret = deflateInit(&strm, Z_BEST_COMPRESSION);
    if (ret != Z_OK)
    {
        LOG(ERROR) << "deflateInit error. ret:" << ret;
        return false;
    }

    std::stringstream ss;
    size_t pos = 0;
    int flush;
    size_t pb_len = pb_data.size();
    /* compress until end of file */
    do {
        std::size_t length = pb_data.copy((char*)in, CHUNK, pos);
        pos += length;
        flush = (pos >= pb_len ? Z_FINISH : Z_NO_FLUSH);
        strm.avail_in = length;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = deflate(&strm, flush);    /* no bad return value */
            // assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            if (ret == Z_STREAM_ERROR)
            {
                LOG(ERROR) << "compress error. ret:" << ret;
                deflateEnd(&strm);
                return false;
            }
            unsigned have = CHUNK - strm.avail_out;
            ss << string((const char*)out, have);
        } while (strm.avail_out == 0);
        // assert(strm.avail_in == 0);     /* all input will be used */
        if (strm.avail_in != 0)
        {
            LOG(ERROR) << "compress error. avail_in:" << strm.avail_in;
            deflateEnd(&strm);
            return false;
        }

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    // assert(ret == Z_STREAM_END);        /* stream will be complete */
    if (ret != Z_STREAM_END)
    {
        LOG(ERROR) << "compress error. ret:" << ret;
        deflateEnd(&strm);
        return false;
    }

    output = ss.str();
    /* clean up and return */
    (void)deflateEnd(&strm);
    return true;
}


InPack::InPack()
{
	m_data = NULL; // 消息包
	m_pb_data = NULL; // protobuf数据
	m_data_len = 0; // 消息包长度
	m_pb_data_len = 0; // protobuf数据长度
	m_uid = 0; // uid
	m_roomid = 0; //roomid
}

InPack::InPack(const char* data, uint32_t size)
{
	reset(data, size);
}

// 解析出协议名称的长度, 协议名称, protobuf数据的长度, protobuf数据
bool InPack::decode_stream(const char* data, uint32_t size)
{
	// 首先解析出协议名称的长度
	//uint16_t type_name_len = *(uint16_t*)data; // 取出uint16_t(2)大小的前面的数据得到协议名称的长度

	uint16_t type_name_len;

	memcpy(&type_name_len, data, sizeof(uint16_t));
	

	type_name_len = ntohs(type_name_len);
	uint32_t total_head_size = kTypeNameLen + type_name_len + kRoomidLen;
	// LOG(INFO) << __func__
    //     << " decode_stream type_name_len = " << type_name_len
    //     << " total_head_size = " << total_head_size
    //     << " size:" << size;

	if (total_head_size > size || type_name_len > kMaxTypeNameLen)
	{
		log_error("pack type error len:%u datasize:%d", type_name_len, size - kTypeNameLen);
		// LOG(INFO)<< "pack type error len, datasize" <<  type_name_len << "," << size - kTypeNameLen;
		return false;
	}

	m_type_name.assign(data + kTypeNameLen, data + kTypeNameLen + type_name_len); // 解析协议名称
	// LOG(INFO) << "decode_stream m_type_name = " << m_type_name;
	int32_t roomid;
	memcpy(&roomid, data + kTypeNameLen + type_name_len, sizeof(int32_t));
	m_roomid = ntohl(roomid);
	// LOG(INFO) << "decode_stream roomid = " << m_roomid;

	m_pb_data = data + kTypeNameLen + type_name_len + kRoomidLen; // 解析pb数据
	m_pb_data_len = size - kTypeNameLen - type_name_len - kRoomidLen; // 解析pb数据长度
	// LOG(INFO) << "m_pb_data_len = " << m_pb_data_len;

	return true;
}

// 没有uid的
bool InPack::reset(const char* data, uint32_t size)
{
	// LOG(INFO) << "InPack reset size = " << size;
	m_data = data;
	if (size == 0)
	{
		m_data_len = strlen(data);
		size = m_data_len;
	}
	else
		m_data_len = size;

	// LOG(INFO) << "InPack reset m_data_len = " << m_data_len;

	return decode_stream(data, size);
}

//
bool InPack::out_reset(const char* data, uint32_t size)
{
	m_data = data;
	if (size == 0)
	{
		m_data_len = strlen(data);
		size = m_data_len;
	}
	else
		m_data_len = size;

	uint32_t pack_head;			// 消息的总长度(4), see new_outpack
	memcpy(&pack_head, data, sizeof(uint32_t));
	pack_head = ntohl(pack_head);

//	LOG(INFO) << "InPack out_reset pack_head = " << pack_head;

	data += sizeof(pack_head);
	size -= sizeof(pack_head);

	return decode_stream(data, size);
}

// 有uid的首先解析出uid,然后再reset解析出消息的其他部分
bool InPack::inner_reset(const char* cdata, uint32_t size)
{
	//LOG(INFO) << "InPack inner_reset size = " << size;
	char* data = const_cast<char*>(cdata);
	m_data = data;
	if (size == 0)
	{
		m_data_len = strlen(data);
		size = m_data_len;
	}
	else
		m_data_len = size;

	//解出uid
	uint64_t uid = *(uint64_t*)data;

	//LOG(INFO) << "InPack::inner_reset uid = " << uid;

	m_uid = ntoh64(&uid);
	//LOG(INFO) << "InPack::inner_reset ntoh64(&uid) = " << m_uid;

	data += sizeof(m_uid);
	size -= sizeof(m_uid);

	return decode_stream(data, size);
}

//得到pb包
Message* InPack::create_message()
{
	Message* message = NULL;
	const Descriptor* descriptor = DescriptorPool::generated_pool()->FindMessageTypeByName(m_type_name);
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
				log_error("pb parse error type:%s", m_type_name.c_str());
			}
		}
	}
	return message;
}

void InPack::test()
{
	log_debug("m_data_len:%d m_pb_data_len:%d m_type_name:%s roomid:%d", m_data_len, m_pb_data_len, m_type_name.c_str(), m_roomid);
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

//todo: 20210630
void OutPack::new_outpack(char* &result, uint32_t& size, uint32_t sub_type, const int32_t& roomid) //【消息的总长度】【 htons 协议名称长度】【协议名称】【roomid】【数据】
{
	//SUBTYPE_AGENT
	//LOG(INFO) << "new_outpack m_type_name = " << m_type_name << " sub_type =  " << sub_type;
	//int32_t roomid = 0; //todo: 20210630, 函数OutPack::new_outpack需增加参数
	if (sub_type == 0) {
		int data_len = m_pb_data.size() + kTypeNameLen + m_type_name.size() + kRoomidLen;
		size = data_len + kMessageLen;
		char* data = (char*)skynet_malloc(size);
		result = data;

		uint32_t pack_head = htonl(data_len);
		memcpy(data, &pack_head, kMessageLen); // 消息的总长度(4)


		uint16_t type_head = htons(m_type_name.size());
		memcpy(data + kMessageLen, &type_head, kTypeNameLen); // 协议名称长度(2)


		memcpy(data + kMessageLen + kTypeNameLen, m_type_name.c_str(), m_type_name.size()); // 协议名称

		int32_t head_roomid = htonl(roomid);
		memcpy(data + kMessageLen + kTypeNameLen + m_type_name.size(), &head_roomid, kRoomidLen); // 消息的总长度(4)

		memcpy(data + kMessageLen + kTypeNameLen + m_type_name.size() + kRoomidLen, m_pb_data.c_str(), m_pb_data.size()); // 数据

	}
	else if(sub_type == SUBTYPE_AGENT){
		int data_len = kTypeNameLen + m_type_name.size() + m_pb_data.size() + kRoomidLen;
		
		size = data_len + sizeof(uint32_t) + kMessageLen;
		//LOG(INFO) << "size = " << size;

		char* data = (char*)skynet_malloc(size);
		result = data;

		uint32_t htonl_sub_type = htonl(SUBTYPE_AGENT);
		memcpy(data, &htonl_sub_type, sizeof(uint32_t)); // sub_type

		uint32_t pack_head = htonl(data_len);
		memcpy(data + sizeof(uint32_t), &pack_head, kMessageLen); // 消息的总长度(4)


		uint16_t type_head = htons(m_type_name.size());
		memcpy(data + sizeof(uint32_t) + kMessageLen, &type_head, kTypeNameLen); // 协议名称长度(2)


		memcpy(data + sizeof(uint32_t) + kMessageLen + kTypeNameLen, m_type_name.c_str(), m_type_name.size()); // 协议名称

		int32_t head_roomid = htonl(roomid);
		memcpy(data + sizeof(uint32_t) + kMessageLen + kTypeNameLen + m_type_name.size(), &head_roomid, kRoomidLen); // roomid

		memcpy(data + sizeof(uint32_t) + kMessageLen + kTypeNameLen + m_type_name.size() + kRoomidLen, m_pb_data.c_str(), m_pb_data.size()); // 数据
	}

	
}


void OutPack::new_innerpack(char* &result, uint32_t& size, uint64_t uid, const int32_t& roomid)
{
	//int32_t roomid = 0; //todo: 20210630, 函数OutPack::new_innerpack需增加参数
	int data_len =  sizeof(uid) + kTypeNameLen + m_type_name.size() + m_pb_data.size() + kRoomidLen;

	size = data_len;
	

	char* data = (char*)skynet_malloc(size);

	result = data;

	
	uint64_t hton64_uid = hton64(&uid);
	//LOG(INFO) << "new_innerpack uid = " << uid;
	//LOG(INFO) << "new_innerpack hton64_uid = " << hton64_uid;
	
	memcpy(data, &hton64_uid, sizeof(uid)); // uid


	uint16_t htonsTypeName = htons(m_type_name.size());
	

	memcpy(data  + sizeof(uid), &htonsTypeName, kTypeNameLen); // namelen


	memcpy(data  + sizeof(uid) + kTypeNameLen, m_type_name.c_str(), m_type_name.size()); // pbname

	int32_t head_roomid = htonl(roomid);
	memcpy(data + sizeof(uid) + kTypeNameLen + m_type_name.size(), &head_roomid, kRoomidLen); // roomid

	memcpy(data  + sizeof(uid) + kTypeNameLen + m_type_name.size() + kRoomidLen, m_pb_data.c_str(), m_pb_data.size()); // pbdata
}

void OutPack::new_innerpack_type(char* &result, uint32_t& size, uint64_t uid, uint32_t type, const int32_t& roomid)// 【sub_type】【uid】【协议名称长度】【协议名称】【roomid】【数据】
{
	//int32_t roomid = 0; //todo: 20210630, 函数OutPack::new_innerpack_type需增加参数
	int data_len = sizeof(uint32_t) + sizeof(uid) + kTypeNameLen + m_type_name.size() + m_pb_data.size() + kRoomidLen;

	size = data_len;
	

	char* data = (char*)skynet_malloc(size);

	result = data;

	
	uint32_t htonl_sub_type = htonl(type); 
	//LOG(INFO) << "new_innerpack type = " << type;
	//LOG(INFO) << "new_innerpack htonl_sub_type = " << htonl_sub_type;

	memcpy(data, &htonl_sub_type, sizeof(uint32_t)); // sub_type
	
	uint64_t hton64_uid = hton64(&uid);
	//LOG(INFO) << "new_innerpack uid = " << uid;
	//LOG(INFO) << "new_innerpack hton64_uid = " << hton64_uid;
	
	memcpy(data + sizeof(uint32_t), &hton64_uid, sizeof(uid)); // uid


	uint16_t htonsTypeName = htons(m_type_name.size());
	

	memcpy(data + sizeof(uint32_t) + sizeof(uid), &htonsTypeName, kTypeNameLen); // namelen


	memcpy(data + sizeof(uint32_t) + sizeof(uid) + kTypeNameLen, m_type_name.c_str(), m_type_name.size()); // pbname

	int32_t head_roomid = htonl(roomid);
	memcpy(data + sizeof(uint32_t) + sizeof(uid) + kTypeNameLen + m_type_name.size(), &head_roomid, kRoomidLen); // roomid


	memcpy(data + sizeof(uint32_t) + sizeof(uid) + kTypeNameLen + m_type_name.size() + kRoomidLen, m_pb_data.c_str(), m_pb_data.size()); // pbdata

	
}

bool OutPack::zip_reset(const Message& msg)
{
    if (!msg.SerializeToString(&m_pb_data)) // 把protobuf消息msg序列化为string并保存到m_pb_data中
    {
        log_error("serialize error\n");
        return false;
    }

    // uint64_t now = CTimeUntil::now_tick_us();
    std::string output;
    if (!zlib_compress(m_pb_data, output))
    {
        log_error("compress error\n");
        return false;
    }

    // uint64_t t = CTimeUntil::now_tick_us() - now;
    // LOG(INFO) << "zip_reset pb_name: " << msg.GetTypeName() << " " << m_pb_data.size() << " " << output.size() << " " << t;

    m_pb_data = output;
    m_type_name = msg.GetTypeName(); // 获取协议名称
    return true;
}

bool OutPack::zip_reset(const std::string& msg, int32_t& roomid)
{
    // datalen + namelen + name + roomid + pbdata
    const char* data = msg.data();
    uint32_t total_len = msg.size();

    uint32_t msglen = 0;
    memcpy(&msglen, data, kMessageLen);
    msglen = ntohl(msglen);

    uint16_t namelen = 0;
    memcpy(&namelen, data + kMessageLen, kTypeNameLen);
    namelen = ntohs(namelen);

    // 协议名
    if (namelen >= 1024)
    {
        char* namebuf = (char*)skynet_malloc(namelen);
        memcpy(namebuf, data + kMessageLen + kTypeNameLen, namelen);
        m_type_name = string(namebuf, namelen);
        skynet_free(namebuf);
    }
    else
    {
        char namebuf[1024] = {0};
        memcpy(namebuf, data + kMessageLen + kTypeNameLen, namelen);
        m_type_name = string(namebuf, namelen);
    }

    memcpy(&roomid, data + kMessageLen + kTypeNameLen + namelen, kRoomidLen);
    roomid = ntohl(roomid);

    const char* rowmsg = data + kMessageLen + kTypeNameLen + namelen + kRoomidLen;
    int row_msg_len = msglen - (kTypeNameLen + namelen + kRoomidLen);

    // TOTEST
    // LOG(INFO) << "zip_reset pb_name:" << m_type_name << " msglen:" << msglen
    //     << " namelen:" << namelen << " roomid:" << roomid << " row_msg_len:" << row_msg_len << " total_size:" << msg.size();
    if (total_len != kMessageLen + msglen)
    {
        LOG(ERROR) << "zip_reset parse error. total_len: " << total_len << " cal_len:" << kMessageLen + msglen;
    }
    //---

    // uint64_t now = CTimeUntil::now_tick_us(); // for test
    std::string input = string(rowmsg, row_msg_len);
    if (!zlib_compress(input, m_pb_data))
    {
        log_error("compress error m_type_name:%s\n", m_type_name.c_str());
        return false;
    }

    // 测试代码
    // uint64_t t = CTimeUntil::now_tick_us() - now;
    // LOG(INFO) << "zip_reset pb_name: " << m_type_name << " " << row_msg_len << " " << m_pb_data.size() << " " << t;

    return true;
}

void OutPack::test()
{
	log_debug("test outputpack:  typelen:%d datalen:%d typename:%s", (int)m_type_name.size(), (int)m_pb_data.size(), m_type_name.c_str());
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

//todo: 20210630
void serialize_msg(const Message& msg, char* &result, uint32_t& size, uint32_t sub_type, const int32_t& roomid)
{
	OutPack opack;
	opack.reset(msg);
	opack.new_outpack(result, size, sub_type, roomid);
}

void serialize_msg_with_compress(const Message& msg, char* &result, uint32_t& size, const int32_t& roomid)
{
    OutPack opack;
    opack.zip_reset(msg);
	int32_t compress_roomid = (1<<31) | roomid;
	// LOG(INFO) << "YAO_TEST serialize_msg_with_compress roomid:" << roomid << ",compress:" << compress_roomid;
    opack.new_outpack(result, size, 0, compress_roomid);
}

void compress_row_msg(const std::string& msg, char* &result, uint32_t& size)
{
    OutPack opack;
    int32_t roomid;
    opack.zip_reset(msg, roomid);
	int32_t compress_roomid = (1<<31) | roomid;
	// LOG(INFO) << "YAO_TEST compress_row_msg roomid :" << roomid << ",compress:" << compress_roomid;
    opack.new_outpack(result, size, 0, compress_roomid);
}

//todo: 20210630
void serialize_imsg(const Message& msg, char* &result, uint32_t& size, uint64_t uid, const int32_t& roomid)
{
	OutPack opack;
	opack.reset(msg);
	//LOG(INFO) << "serialize_imsg msg typename = " << msg.GetTypeName();
	opack.new_innerpack(result, size, uid, roomid);
}

void serialize_imsg_type(const Message& msg, char* &result, uint32_t& size, uint64_t uid, uint32_t sub_type, const int32_t& roomid)
{
	OutPack opack;
	opack.reset(msg);
	//LOG(INFO) << "serialize_imsg msg typename = " << msg.GetTypeName();
	opack.new_innerpack_type(result, size, uid, sub_type, roomid);
}



