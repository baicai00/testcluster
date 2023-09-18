require "skynet.manager"
local skynet = require "skynet"
local cluster = require "bee_cluster"
local protobuf = require "protobuf"

local gtables = {}

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

skynet.init(function ()
    local lua_root = skynet.getenv("lua_root") .. "protocol/pb/"
    local pb_names = gtables.get_pb_names()
    gtables.register_pb(lua_root, pb_names)
end)

skynet.start(function ()

    cluster.register("mail_master", skynet.self())
    cluster.open("mail_master_6")

    local debug_port = assert(skynet.getenv("debug_port"))
    skynet.newservice("debug_console", debug_port)

    local proxy = skynet.newservice("CServiceProxy")

    local sendline = string.format("%d", proxy)
    local mail = assert(skynet.launch("mail", sendline))
    skynet.name(".mail", mail)

    skynet.dispatch("lua", function (session , source, sub_type, ...)
        if sub_type == "lua" then
            local args = table.pack(...)
            local cmd = args[1]
            -- local f = assert(CMD[cmd])
            -- local ret = f(table.unpack(args, 2))
            -- if session ~= 0 then
            --     skynet.ret(skynet.pack(ret))
            -- end
        elseif sub_type == "text" then
            local args = table.pack(...)
            local netmsg = args[1]
            if netmsg ~= nil then
                skynet.redirect(mail, source, "text", session, netmsg)
            else
                beelog_error("text netmsg is nil")
            end
        elseif sub_type == "rpc_response" then
            local args = table.pack(...)
            local netmsg = args[1]
            if netmsg ~= nil then
                skynet.redirect(mail, source, "rpc_response", 0, netmsg)
            else
                beelog_error("rpc_response netmsg is nil")
            end
        end
    end)
end)