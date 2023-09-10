require "skynet.manager"
local skynet = require "skynet"
local cluster = require "bee_cluster"
local protopack = require "protopack"
local protobuf = require "protobuf"
local dispatch= require "dispatcher"


local dispatcher
local gtables = {}
local sub_services = {}
local pb_list = {}
local naming_status_pb_name = {}

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

function gtables.get_addrs(uid)
    local addrs, size = {}, #sub_services
    if not uid or uid < 0 then
        table.insert(addrs, sub_services[math.random(1, size)])
    elseif uid > 0 then
        table.insert(addrs, sub_services[1 + uid % size])
    elseif uid == 0 then
        -- broadcast to all servcies
        addrs = table.copy(sub_services)
    end
    return addrs
end

function gtables.get_pb_names()
    local pb_names = {
        "inner.pb",
    }
    return pb_names
end

skynet.init(function()
    local lua_root = skynet.getenv("lua_root") .. "protocol/pb/"
    local pb_names = gtables.get_pb_names()
    gtables.register_pb(lua_root, pb_names)

    pb_list = {
        "pb.iTestPingActivityMsg",
        "pb.iTestPingActivityREQ",
    }
end)

skynet.start(function()

    dispatcher = dispatch.new(gtables)
    dispatcher:regs(protobuf, pb_list)

    cluster.register("activity_master", skynet.self())
    cluster.open("activity_master_5")

    local debug_port = assert(skynet.getenv("debug_port"))
    skynet.newservice("debug_console", debug_port)

    for i = 1, 1 do
        table.insert(sub_services, skynet.newservice("activity", i, table.concat(pb_list, ",")))
    end

    skynet.dispatch("lua", function (session , source, sub_type, ...)
        if sub_type == "lua" then
            local args = table.pack(...)
            local cmd = args[1]
            local uid = args[2]
            -- beelog_info("TEST lua cmd:", cmd, "uid:", uid)
            local addrs = gtables.get_addrs(uid)
            for _, addr in pairs(addrs) do
                skynet.redirect(addr, source, "lua", session, skynet.pack(cmd, ...))
            end
        elseif sub_type == "text" then
            local args = table.pack(...)
            local netmsg = args[1]
            if netmsg ~= nil then
                local datalen = string.len(netmsg) - 4 - 8 - 2 - 4 -- subtype uid namelen roomid
                local uid = string.unpack("> L", netmsg, 5)

                beelog_info("session:", session, "source:", source)

                local temp_uid, temp_name, temp_msg = protopack.unpack_raw(netmsg, protobuf)
                if naming_status_pb_name[temp_name] then
                    dispatcher:dispatch(temp_uid, temp_name, temp_msg, session, source, protobuf)
                    return
                end

                local addrs = gtables.get_addrs(uid)
                for _, addr in ipairs(addrs) do
                    skynet.redirect(addr, source, "text", session, netmsg)
                end
            end
        end
    end)
end)