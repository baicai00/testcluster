require "skynet.manager"
local skynet = require "skynet"
local cluster = require "cluster"
local protopack = require "protopack"
local protobuf = require "protobuf"

local gtables = {}

function gtables.register_pb(root, names)
    for _, name in ipairs(names) do
        local f = assert(io.open(root .. name, "rb"))
        local pb_buffer = f:read "*a"
        protobuf.register(pb_buffer)
        f:close()
    end
end

skynet.start(function()
    skynet.error("start node1==========")

    cluster.open("node1_1")

    -- local lua_root = skynet.getenv("lua_root") .. "protocol/pb/"
    -- local pb_names = {
    --     "inner.pb",
    -- }
    -- gtables.register_pb(lua_root, pb_names)

    -- local test_msg = {id = 1, name = "helloworld"}
    -- local pack_msg = protopack.pack_raw(protopack.SUBTYPE_PROTOBUF, 0, "pb.Test", test_msg, protobuf)
    -- local uid, name, unpack_msg = protopack.unpack_raw(pack_msg, protobuf)
    -- skynet.error("name:", name, "unpack_msg:", tostring(unpack_msg))

    while true do
        -- local status, proxy = pcall(cluster.proxy, "node2_3", "@node2")
        -- if status then
        --     local ret = skynet.call(proxy, "lua", "ping")
        --     skynet.error(ret)
        -- end
        local status, node2 = pcall(cluster.query, "node2_3", "node2")
        if not status then
            skynet.error("pcall cluster.call error:", node2)
        else
            local ret = cluster.call("node2_3", node2, "ping")
            skynet.error(ret)
        end
        skynet.sleep(100)
    end
end)