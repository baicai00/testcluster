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
    beelog_info("start shop ===============")

    cluster.register("shop_master", skynet.self())
    cluster.open("shop_master_4")
    -- skynet.register(".shop_master")

    local debug_port = assert(skynet.getenv("debug_port"))
    skynet.newservice("debug_console", debug_port)

    local proxy = skynet.newservice("CServiceProxy")

    local sendline = string.format("%d", proxy)
    local shop = assert(skynet.launch("shop", sendline))
    -- cluster.register("shop", shop)

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
                beelog_info("TEST recv session:", session)
                skynet.redirect(shop, source, "text", session, netmsg)
            else
                beelog_error("netmsg is nil")
            end
        end
    end)
end)