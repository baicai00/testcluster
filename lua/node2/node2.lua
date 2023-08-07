require "skynet.manager"
local skynet = require "skynet"
local cluster = require "cluster"
local CMD = {}

-- skynet.register_protocol {
--     name = "text",
--     id = skynet.PTYPE_TEXT,
--     pack = function(...) return ... end,
--     unpack = skynet.tostring,
-- }

function CMD.ping()
    skynet.error("TEST ping time:", os.time())
    return "pong" .. os.time()
end


skynet.start(function()
    skynet.error("start node2==========")

    cluster.register("node2", skynet.self())
    cluster.open("node2_3")

    skynet.dispatch("lua", function (session , source, cmd, ...)
        local f = assert(CMD[cmd])
        local ret = f(...)
        if session ~= 0 then
            skynet.ret(skynet.pack(ret))
        end
    end)

    -- skynet.dispatch("text", function(session, source, netmsg)
        
    -- end)
end)