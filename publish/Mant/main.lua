--
-- Date: 5/15/20
-- Time: 6:36 PM
--
package.path = package.path .. ";./Mant/?.lua;"

local config_path = "/work/env/"
local config_name = "config.node"
local dev_hostname = "local-dev"

local export = {
    service_name = "main",
    harbor = 1,
    daemon = true,
    lua_service = "main",
    domain = 0,
    master = "127.0.0.1",
    inner_ip = "10.100.0.243",
    lroomsvr_type = "default",
    lroomsvr_version = 1,
    clear_naming = false,
    up_status = "none"
}


local MTools = require("MTools")
local tools = MTools:new(export, config_path, config_name, dev_hostname)


local function main(action, ...)
    if not action then action = "none__action" end
    if tools[action] then
        tools[action](tools, ...)
    else
        print("ERROR action:" .. action)
        tools:usage()
    end
end

main(arg[1], arg[2], arg[3], arg[4])
