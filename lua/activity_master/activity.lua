require 'skynet.manager'
local skynet = require 'skynet'
local protobuf = require "protobuf"
local protopack = require "protopack"
local dispatch= require "dispatcher"


local sub_service_id, init_params = ...
sub_service_id = tonumber(sub_service_id)

local dispatcher
local gtables = {}
local CMD = {}


skynet.register_protocol {
    name = "text",
    id = skynet.PTYPE_TEXT,
    pack = function(...) return ... end,
    unpack = skynet.tostring,
}

skynet.register_protocol {
    name = "rpc_response",
    id = skynet.PTYPE_RESPONSE,
    pack = function(...) return ... end,
    unpack = skynet.tostring,
}


function gtables.register_pb(root, names)
    for _, name in ipairs(names) do
        local f = assert(io.open(root .. name, "rb"))
        local pb_buffer = f:read "*a"
        protobuf.register(pb_buffer)
        f:close()
    end
end

function gtables.get_pb_names()
    local pb_names = {
        "inner.pb",
    }
    return pb_names
end

function CMD.test_ping_activity(_, context)
    beelog_info("TEST test_ping_activity context:", context, "sub_service_id:", sub_service_id)
    return "test_ping_activity pong " .. os.time()
end

function gtables.iTestPingActivityMsg(uid, msg)
    beelog_info("TEST iTestPingActivityMsg uid:", uid, "sub_service_id:", sub_service_id, "msg:", tostring(msg))
end

function gtables.iTestPingActivityREQ(uid, msg)
    beelog_info("TEST iTestPingActivityREQ uid:", uid, "sub_service_id:", sub_service_id, "msg:", tostring(msg))
    local rsp = {pong_msg = "Hello shop, I am activity from iTestPingActivityREQ"}
    return "iTestPingActivityRSP", rsp
end

skynet.init(function ()
    local lua_root = skynet.getenv("lua_root") .. "protocol/pb/"
    local pb_names = gtables.get_pb_names()
    gtables.register_pb(lua_root, pb_names)
end)

skynet.start(function ()

    skynet.dispatch("text", function(session, source, netmsg)
        if netmsg ~= nil then
            local uid, name, msg = protopack.unpack_raw(netmsg, protobuf)
            dispatcher:dispatch(uid, name, msg, session, source, protobuf)
        end
    end)

    skynet.dispatch("lua", function(session, address, cmd, ...)
        local f = CMD[cmd]
        if f then
            if session ~= 0 then
                skynet.ret(skynet.pack(f(address, ...)))
            else
                f(address, ...)
            end
        elseif gtables[cmd] then
            f = gtables[cmd]
            if session ~= 0 then
                skynet.ret(skynet.pack(f(...)))
            else
                f(...)
            end
        else
            assert(false, "no cmd" .. cmd)
        end
    end)

    -- 注册协议处理函数
    local pb = string.split(init_params, ",")
    dispatcher = dispatch.new(gtables)
    dispatcher:regs(protobuf, pb)
end)