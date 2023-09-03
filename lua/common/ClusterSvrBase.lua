local skynet = require "skynet"
local cluster = require "bee_cluster"
local protopack = require "protopack"
local protobuf = require "protobuf"
local ClusterSvrBase = class2()


function ClusterSvrBase:ctor() end

function ClusterSvrBase:init()
end

function ClusterSvrBase:get_pb_names()
    local pb_names = {
        "inner.pb",
        "test.pb",
    }
    return pb_names
end

function ClusterSvrBase:register_pb(root, names)
    for _, name in ipairs(names) do
        local f = assert(io.open(root .. name, "rb"))
        local pb_buffer = f:read "*a"
        protobuf.register(pb_buffer)
        f:close()
    end
end




return ClusterSvrBase