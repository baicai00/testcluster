local skynet = require "skynet"

--[[
c++服务过来的protobuf消息通信的协议
内部通信的协议:
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
    【roomid】        占8字节
    【session】       占8字节
    【数据】

]]

-- 消息的格式: 
-- 与外部通信的协议：消息的总长度(4) + 类型名长度(2) + 类型名(string) + roomid(4)+ pb数据
-- 与
-- '<' little endian '>' big endian

local M = {}

M.SUBTYPE_PROTOBUF = 16
M.SUBTYPE_PLAIN_TEXT = 19
M.SUBTYPE_RPC_CLIENT = 17
M.SUBTYPE_RPC_SERVER = 18
M.SUBTYPE_AGENT = 20
M.SUBTYPE_SYSTEM = 21

-- 解包c++发来的protobuf消息 
function M.unpack_raw_cluster(str, protobuf)
    --[[
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

    -- 解析节点与服务名
    local remote_node_len,remote_service_len,source_node_len,source_service_len = string.unpack("> H H H H", str)
    local fmt = string.format("> c%d c%d c%d c%d", remote_node_len, remote_service_len, source_node_len, source_service_len)
    -- 前8个字节是长度，名字从第9个字节开始
    local remote_node_name, remote_service_name, source_node_name, source_service_name = string.unpack(fmt, str, 9)
    -- skynet.error("TEST unpack_raw_cluster ", remote_node_name, remote_service_name, source_node_name, source_service_name)

    -- sub_type, uid, pb_name_len
    local len = 2+2+2+2+remote_node_len+remote_service_len+source_node_len+source_service_len
    local sub_type, uid, namelen = string.unpack("> I L H", str, len+1)
    -- skynet.error("TEST unpack_raw_cluster ", sub_type, uid, namelen)

    -- pbname, roomid, session, pbstr
    len = 2+2+2+2+remote_node_len+remote_service_len+source_node_len+source_service_len+4+8+2
    local pbstr_len = string.len(str) - len - namelen - 4 - 8
    fmt = string.format("> c%d I L c%d", namelen, pbstr_len)
    local pbname, roomid, session, pbstr = string.unpack(fmt, str, len+1)
    local msg, errmsg = protobuf.decode(pbname, pbstr)
    if not msg then
        skynet.error("ERROR decode msg failed. pbname:", pbname, "errmsg:", errmsg)
    end

    local ret = {
        remote_node_name = remote_node_name,
        remote_service_name = remote_service_name,
        source_node_name = source_node_name,
        source_service_name = source_service_name,
        sub_type = sub_type,
        uid = uid,
        pbname = pbname,
        roomid = roomid,
        session = session,
        msg = msg,
    }
    return ret
end

-- 从C++消息中解析出远程节点名与远程服务名
function M.unpack_raw_remote_name(str)
	local remote_node_len = string.unpack("> H", str)
	local remote_service_len = string.unpack("> H", str, 3)
	local fmt = string.format("> c%d c%d", remote_node_len, remote_service_len)
	local remote_node_name, remote_service_name = string.unpack(fmt, str, 9)
	return remote_node_name, remote_service_name
end

-- 打包内部protobuf消息 发送给c++服务
function M.pack_raw(type, uid, name, msg, protobuf, roomid, session)
    -- skynet.error("TEST pack_raw session:", session)
    local pbstr = protobuf.encode(name, msg)
    local pblen = string.len(pbstr)
    local namelen = #name
    local head_roomid = roomid or 0
    local head_session = session or 0
    local fmt = string.format(">I L L H c%d I c%d", namelen, pblen)
    local str = string.pack(fmt, type, uid, head_session, namelen, name, head_roomid, pbstr)
    return str
end

return M
