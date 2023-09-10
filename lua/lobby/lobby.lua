require "skynet.manager"
local skynet = require "skynet"
local cluster = require "bee_cluster"
local protopack = require "protopack"
local protobuf = require "protobuf"

local gtables = {}
local service_handle = {}
local service_proxy = {}

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

function gtables.get_service_handle(node_name, service_name)
    local handle = service_handle[service_name]
    if handle then
        return handle
    end

    local status, handle = pcall(cluster.query, node_name, service_name)
    if not status then
        beelog_error("ERROR pcall cluster.query node:", node_name, "service_name:", service_name, "error:", handle)
        return nil
    else
        service_handle[service_name] = handle
        return handle
    end
end

function gtables.get_service_proxy(node_name, service_name)
    local proxy = service_proxy[service_name]
    if proxy then
        return proxy
    end

    local name = node_name .. "@" .. service_name
    local status, proxy = pcall(cluster.proxy, name)
    if not status then
        beelog_error("ERROR pcall cluster.proxy node:", node_name, "service_name:", service_name, "error:", proxy)
        return nil
    else
        service_proxy[service_name] = proxy
        return proxy
    end
end

function gtables.test_ping_activity()
    local handle = gtables.get_service_handle("activity_master_5", "activity_master")
    if not handle then
        return
    end

    local uid = 6
    local ret = cluster.call("activity_master_5", handle, "lua", "test_ping_activity", uid, "hello activity_master, I am lobby")
    beelog_info(ret)

    -- test protobuf
    local msg = {ping_msg = "proto hello activity_master"}
    uid = 5
    local pack_msg = protopack.pack_raw(protopack.SUBTYPE_PROTOBUF, uid, "pb.iTestPingActivityMsg", msg, protobuf)
    cluster.send("activity_master_5", handle, "text", pack_msg)
end

function gtables.test_ping_activity_with_proxy()
    local proxy = gtables.get_service_proxy("activity_master_5", "activity_master")
    if not proxy then
        return
    end

    local uid = 6
    local ret = skynet.call(proxy, "lua", "lua", "test_ping_activity", uid, "hello activity_master, I am lobby")
    beelog_info(ret)

    -- test protobuf
    local msg = {ping_msg = "proto hello activity_master"}
    uid = 5
    local pack_msg = protopack.pack_raw(protopack.SUBTYPE_PROTOBUF, uid, "pb.iTestPingActivityMsg", msg, protobuf)
    skynet.send(proxy, "lua", "text", pack_msg)
end

function gtables.test_ping_lpersistence()
    local handle = gtables.get_service_handle("lpersistence_3", "lpersistence")
    if not handle then
        return
    end

    local ret = cluster.call("lpersistence_3", handle, "lua", "ping")
    beelog_info(ret)

    -- test protobuf
    local test_msg = {id = 1, name = "helloworld"}
    local pack_msg = protopack.pack_raw(protopack.SUBTYPE_PROTOBUF, 0, "pb.TestMsg", test_msg, protobuf)
    cluster.send("lpersistence_3", handle, "text", pack_msg)
end

function gtables.test_ping_mysqlpool()
    local handle = gtables.get_service_handle("lpersistence_3", "mysqlpool")
    if not handle then
        return
    end

    local sql = "select * from user where uid = 10076447"
    local ret = cluster.call("lpersistence_3", handle, "lua", "execute", sql)
    beelog_info(tostring(ret))
end

function gtables.test_ping_shop()
    local handle = gtables.get_service_handle("shop_master_4", "shop_master")
    if not handle then
        return
    end

    local test_msg = {ping_msg = "hello shop, I am lobby"}
    local pack_msg = protopack.pack_raw(protopack.SUBTYPE_PROTOBUF, 0, "pb.iTestPingShopREQ", test_msg, protobuf)
    cluster.send("shop_master_4", handle, "text", pack_msg)
end

skynet.start(function()

    cluster.open("lobby_1")
    cluster.register("lobby", skynet.self())

    local debug_port = assert(skynet.getenv("debug_port"))
    skynet.newservice("debug_console", debug_port)

    local lua_root = skynet.getenv("lua_root") .. "protocol/pb/"
    local pb_names = gtables.get_pb_names()
    gtables.register_pb(lua_root, pb_names)

    skynet.sleep(300)

    while true do
        -- gtables.test_ping_lpersistence()

        -- gtables.test_ping_mysqlpool()

        -- gtables.test_ping_activity()
        -- gtables.test_ping_activity_with_proxy()

        gtables.test_ping_shop()

        skynet.sleep(100)
    end
end)