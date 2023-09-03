require "skynet.manager"
local skynet = require "skynet"
local cluster = require "bee_cluster"
local protopack = require "protopack"
local protobuf = require "protobuf"


skynet.register_protocol {
    name = "text",
    id = skynet.PTYPE_TEXT,
    pack = function(...) return ... end,
    unpack = skynet.tostring,
}

skynet.start(function ()
    local service = skynet.getenv("service_name")
    beelog_info("TEST service_name:", service)

    assert(skynet.launch(service))

    skynet.exit()
end)