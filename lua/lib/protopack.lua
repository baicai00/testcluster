local skynet = require "skynet"
--local protobuf = require "protobuf"
-- local tool = require "tool"


-- 根据之前c++服务的protobuf消息通信的协议
-- 消息的格式: 
-- 与外部通信的协议：消息的总长度(4) + 类型名长度(2) + 类型名(string) + roomid(4)+ pb数据
-- 与内部通信的协议: uid(int64 l) + 类型名长度(2) + 类型名(string) + roomid(4) + pb数据


-- '<' little endian '>' big endian

local M = {}

M.SERVICE_NULL = -1
M.SERVICE_NONE = 0
M.SERVICE_ROOM = 2
M.SERVICE_MAIL = 4
M.SERVICE_LHALL 	= 12
M.SERVICE_ACTIVITY  = 14
M.SERVICE_LOBBY = 15
M.SERVICE_FRIEND = 16
M.SERVICE_ANTI_CHEATING = 17
M.SERVICE_GROUP = 18
M.SERVICE_GAME_HELPER = 19
M.SERVICE_LHALL_HELPER = 20
M.SERVICE_MSG_QUEUE = 21
M.SERVICE_ACTIVITY_HELPER = 22
M.SERVICE_ROOM_HELPER = 23

M.SUBTYPE_PROTOBUF = 16  
M.SUBTYPE_PLAIN_TEXT = 19
M.SUBTYPE_RPC_CLIENT = 17 
M.SUBTYPE_RPC_SERVER = 18 
M.SUBTYPE_AGENT = 20
M.SUBTYPE_SYSTEM = 21


-- 打包内部protobuf消息 发送给c++服务
function M.pack_raw(type, uid, name, msg, protobuf, roomid)
	-- skynet.error("type = ", type)
	-- skynet.error("uid = ", uid)
	-- skynet.error("name = ", name)
	local pbstr = protobuf.encode(name, msg)
	local pblen = string.len(pbstr)
	local namelen = #name
	local head_roomid = 0
	if nil == roomid then
		head_roomid = 0
	else
		head_roomid = roomid
	end
	---- skynet.error("namelen = ", namelen)
	local fmt = string.format(">I L H c%d I c%d", namelen, pblen)
	local str = string.pack(fmt, type, uid, namelen, name, head_roomid, pbstr)
	
	return str
end

-- 解包c++发来的protobuf消息 
function M.unpack_raw(str, protobuf)
	-- subtype uid namelen name data
	local datalen = string.len(str) - 4 - 8 - 2 - 4 -- subtype uid namelen roomid
	local uid = string.unpack("> L", str, 5)
	-- skynet.error("uid = ", uid)
	local namelen = string.unpack("> H", str, 13)
	-- skynet.error("namelen = ", namelen)
	local pblen = datalen - namelen -- pbname
	local fmt = string.format("> c%d I c%d", namelen, pblen)
	local name, roomid, pbstr = string.unpack(fmt, str, 15) -- 8 + 2
	local msg = protobuf.decode(name, pbstr)
	
	return uid, name, msg

end

function M.unpack_raw_cluster(str)
	-- remote_node_len,remote_service_len,remote_node_name,remote_service_name,pbdata
	local remote_node_len = string.unpack("> H", str)
	local remote_service_len = string.unpack("> H", str, 3)
	-- local pbstr_len = string.unpack("> L", str, 5)
	local pbstr_len = string.len(str) - 2 - 2 - remote_node_len - remote_service_len
	local fmt = string.format("> c%d c%d c%d", remote_node_len, remote_service_len, pbstr_len)
	-- skynet.error("TTT unpack_raw_cluster remote_node_len:", remote_node_len, "remote_service_len:", remote_service_len, "pbstr_len:",pbstr_len, "len:", string.len(str))
	local remote_node_name, remote_service_name, pbstr = string.unpack(fmt, str, 5)
	return remote_node_name, remote_service_name, pbstr
end

-- 解包客户端通过agent发来的消息
function M.unpack_agent(str,  protobuf)
	-- uid namelen name data
	-- skynet.error("str = ", str)
	local datalen = string.len(str) - 8 - 2 - 4 -- uid namelen roomid
	local uid = string.unpack("> L", str, 1)
	-- skynet.error("unpack_agent uid = ", uid)
	local namelen = string.unpack("> H", str, 9)
	-- skynet.error("namelen = ", namelen)
	local pblen = datalen - namelen -- pbname
	local fmt = string.format("> c%d I c%d", namelen, pblen)
	local name, roomid, pbstr = string.unpack(fmt, str, 11)
	-- skynet.error("pbstr = ", tool.hex(pbstr))
	local msg = protobuf.decode(name, pbstr)
	
	return uid, name, msg
end

-- 打包发送给agent的消息
function M.pack_agent(type, name, msg,  protobuf, roomid)
	-- skynet.error("pack_agent name = ", name)
	local pbstr = protobuf.encode(name, msg)
	-- skynet.error("pack_agent success")
	local pblen = string.len(pbstr)

	local namelen = #name
	local datalen = 2 + namelen + 4 + pblen -- namelen name roomid pb

	if nil == roomid then
		head_roomid = 0
	else
		head_roomid = roomid
	end

	local fmt = string.format("> I I H c%d I c%d", namelen, pblen)
	local str = string.pack(fmt, type, datalen, namelen, name, head_roomid, pbstr)
	
	return str
end

-- 打包发送给客户端的消息 通过socket发送给客户端
function M.pack(name, msg,  protobuf, roomid)
	local pbstr = protobuf.encode(name, msg)
	local pblen = string.len(pbstr)

	local namelen = #name
	local datalen = 2 + namelen + 4 + pblen -- namelen name roomid pb

	if nil == roomid then
		head_roomid = 0
	else
		head_roomid = roomid
	end

	local fmt = string.format("> I H c%d I c%d", namelen, pblen)
	local str = string.pack(fmt, datalen, namelen, name, head_roomid, pbstr)
	
	return str
end

function M.unpack(str,  protobuf)
	-- skynet.error("recv unpack:".. tool.hex(str))
	-- skynet.error("totallen = ", string.len(str))
	local datalen = string.len(str) - 4 - 2 - 4 -- datalen namelen uid
	-- skynet.error("datalen = ", datalen)
	local pos = 5
	local namelen = string.unpack("> H", str, pos)
	-- skynet.error("namelen = ", namelen)
	local pblen = datalen - namelen -- pbname
	-- skynet.error("pblen = ", pblen)

	local fmt = string.format("> c%d I c%d", namelen, pblen)
	local name, roomid, pbstr = string.unpack(fmt, str, 7)
	
	local msg = protobuf.decode(name, pbstr)
	
	return name, msg
end

return M
