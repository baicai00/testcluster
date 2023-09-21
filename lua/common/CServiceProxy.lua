require "skynet.manager"
local skynet = require "skynet"
local cluster = require "bee_cluster"
local protopack = require "protopack_cluster"
local protobuf = require "protobuf"

local gtables = {}
local service_handle = {}

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

-- function gtables.get_pb_names()
--     local pb_names = {
--         "inner.pb",
--     }
--     return pb_names
-- end

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

-- function gtables.test_ping_activity()
--     local handle = gtables.get_service_handle("activity_master_5", "activity_master")
--     if not handle then
--         return
--     end

--     local uid = 6
--     local ret = cluster.call("activity_master_5", handle, "lua", "test_ping_activity", uid, "hello activity_master, I am cserverproxy", os.time())
--     beelog_info(ret)
-- end

skynet.init(function ()
    -- local lua_root = skynet.getenv("lua_root") .. "protocol/pb/"
    -- local pb_names = gtables.get_pb_names()
    -- gtables.register_pb(lua_root, pb_names)
end)

skynet.start(function ()
    skynet.dispatch("text", function (session, source, netmsg)
        if netmsg ~= nil then
            local remote_node_name, remote_service_name = protopack.unpack_raw_remote_name(netmsg)
            beelog_info("TEST cserverproxy recv text:",remote_node_name, remote_service_name)
            local handle = gtables.get_service_handle(remote_node_name, remote_service_name)
            if handle then
                if session == 0 then
                    cluster.send(remote_node_name, handle, "text", netmsg)
                else
                    cluster.send(remote_node_name, handle, "rpc_response", netmsg)
                end
            else
                beelog_error("text handler not found remote_node_name:", remote_node_name, "remote_service_name:", remote_service_name)
            end
        end
    end)

    -- skynet.timeout(300, function ()
    --     while true do
    --         gtables.test_ping_activity()
    --         skynet.sleep(100)
    --     end
    -- end)
end)