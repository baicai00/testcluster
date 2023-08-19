require "skynet.manager"
local skynet = require "skynet"
local cluster = require "bee_cluster"
local protopack = require "protopack"
local protobuf = require "protobuf"
local dispatch= require "dispatcher"

local dispatcher
local CMD = {}
local gtables = {}

skynet.register_protocol {
    name = "text",
    id = skynet.PTYPE_TEXT,
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

function gtables.TestMsg(uid, msg)
    beelog_info("TEST TestMsg uid:", uid, "msg:", tostring(msg))
end

function CMD.ping()
    beelog_info("TEST ping time:", os.time())
    return "pong" .. os.time()
end

skynet.init(function()
    local lua_root = skynet.getenv("lua_root") .. "protocol/pb/"
    local pb_names = {
        "inner.pb",
    }
    gtables.register_pb(lua_root, pb_names)
end)


skynet.start(function()
    beelog_info("start node2==========")

    local pb = {
        "pb.Test",
    }

    dispatcher = dispatch.new(gtables)
    dispatcher:regs(protobuf, pb)

    cluster.register("node2", skynet.self())
    cluster.open("node2_3")

    skynet.newservice("debug_console", 8002)

    skynet.dispatch("lua", function (session , source, cmd, ...)
        local f = assert(CMD[cmd])
        local ret = f(...)
        if session ~= 0 then
            skynet.ret(skynet.pack(ret))
        end
    end)

    skynet.dispatch("text", function(session, source, netmsg)
        if netmsg ~= nil then
            local uid, name, msg = protopack.unpack_raw(netmsg, protobuf)
            dispatcher:dispatch(uid, name, msg, session, source, protobuf)
        end
    end)
end)