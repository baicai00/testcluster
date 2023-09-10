require "skynet.manager"
local skynet = require "skynet"
local cluster = require "bee_cluster"
local protopack = require "protopack"
local protobuf = require "protobuf"

local gtables = {}
local service_handle = {}

skynet.register_protocol {
    name = "text",
    id = skynet.PTYPE_TEXT,
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

skynet.init(function ()
    -- local lua_root = skynet.getenv("lua_root") .. "protocol/pb/"
    -- local pb_names = gtables.get_pb_names()
    -- gtables.register_pb(lua_root, pb_names)
end)

skynet.start(function ()
    skynet.dispatch("text", function (session, source, netmsg)
        if netmsg ~= nil then
            local remote_node_name, remote_service_name, pbstr = protopack.unpack_raw_cluster(netmsg)
            local handle = gtables.get_service_handle(remote_node_name, remote_service_name)
            if handle then
                beelog_info("TEST session:", session)
                if session ~= 0 then
                    cluster.send_with_session(remote_node_name, handle, session, "text", pbstr)
                else
                    cluster.send(remote_node_name, handle, "text", pbstr)
                end
            else
                beelog_error("handler not found remote_node_name:", remote_node_name, "remote_service_name:", remote_service_name)
            end
        end
    end)
end)