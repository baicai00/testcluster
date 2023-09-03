package.path = package.path .. ";./skynet/lualib/?.lua;"
package.path = package.path .. ";./lua/global/?.lua;"
local json = require "json"
require "luaext"

local MTools = class("MTools")

local export
function MTools:ctor(self, export_param, config_path, config_name, dev_hostname)
    export = export_param
    local file = config_path .. config_name
    local f = io.open(file, "rb")
    if not f then
        print("ERR. Can't open " .. file)
    end
    self.template = f:read "*a"
    f:close()

    self.config_path = config_path
    self.config_name = "MT__" .. config_name
    self.dev_hostname = dev_hostname
    self.hostname = string.gsub(self:exec("hostname", false), "\n", "")

    self:exec(string.format("rm %s -f", self.config_name), false)

    self:init()
end

function MTools:init()
    local file = self.config_path .. "server.json"
    local f = io.open(file, "rb")
    if not f then
        print("ERR. Can't open " .. file)
    end

    local servers = f:read "*a"
    f:close()
    servers = json.decode(servers)

    local server, defaut_server
    for _, item in ipairs(servers) do
        if item.hostname == self.hostname then
            server = item
            break
        end
        if item.hostname == self.dev_hostname then
            defaut_server = item
        end
    end

    if not server then server = defaut_server end

    self.server = server
    assert(self.server)

    print(string.format("hostname:%s,  config:%s%s", self.hostname, self.config_path, self.config_name))
    -- print(tostring(server))
end

function MTools:export_to_template(template, config_name)
    local template_cp = table.copy(template)
    for key, value in pairs(export) do
        local old = "$" .. key
        local new = tostring(value)
        template_cp = string.gsub(template_cp, old, new)
    end

    self:writefile(config_name, template_cp)
end

function MTools:writefile(path, content, mode)
    mode = mode or "w+b"
    local file = io.open(path, mode)
    if file then
        if file:write(content) == nil then return false end
        io.close(file)
        return true
    else
        return false
    end
end

function MTools:start_skynet(config_name, service, service_id, log_name)
    local cmd_sub = string.format("./skynet/skynet %s %s", config_name, string.format("-%s -%s", service, service_id))
    local cmd_rm_log = string.format("rm ./%s/*%s%s.* -f && ", log_name, service, service_id)
    local cmd = string.format(" %s ulimit -c unlimited && %s", cmd_rm_log, cmd_sub)
    pcall(self.exec, self, cmd, false)
    print(cmd_sub)
    os.execute("sleep 0.1")
end

function MTools:exec(cmd, log)
    if nil == log then log = true end
    local t = io.popen(cmd)
    local ret = t:read("*all")
    if log == true then print(ret) end
    return ret
end

function MTools:stop_pid(arg, config_name)
    assert(arg)
    local service = arg

    local proc = string.format('echo "`ps aux |grep skynet |grep "%s" |grep -v grep`"', config_name)
    if service ~= "all" then
        proc = string.format(' %s |grep "%s"', proc, service)
    end

    local tmp = string.gsub(self:exec(proc, false), "\n", "\n")
    proc = string.format("echo '%s' |awk '{print $2}'", tmp)

    local pid = string.gsub(self:exec(proc, false), "\n", " ")

    if pid and #pid > 2 then
        os.execute("kill " .. pid)
        print("kill " .. service .. " " .. pid)
    else
        print("no service")
    end
end

function MTools:start(...)
    local args = table.pack(...)
    assert(args[1])
    local daemon = true           -- daemon -f or -d
    local is_bak = 0
    if args[2] and args[2] == "-f" then daemon = false end
    if args[2] and args[2] == "-bak" then is_bak = 1 end
    if args[2] and args[2] == "-notbak" then is_bak = 0 end

    -- export["domain"] = self.server.domain
    -- export["master"] = self.server.master
    -- export["inner_ip"] = self.inner_ip
    export["daemon"] = daemon

    local services = {}

    for _, item in ipairs(self.server.services) do
        local name = item.name
        for _, sub_item in ipairs(item.node_list) do
            if (not sub_item.is_bak and args[1] == "all")
                    or (not sub_item.is_bak and is_bak == 0 and args[1] == name)
                    or (sub_item.is_bak and is_bak == 1 and args[1] == name) then
                table.insert(services, {
                    name = name,
                    service_id = sub_item.service_id,
                    node_tag = sub_item.node_tag,
                    ip = sub_item.ip,
                    port = sub_item.port,
                    debug_port = sub_item.debug_port,
                    online = sub_item.online
                })
            end
        end
    end

    if #services <= 0 then
        print("ERROR service:" .. args[1])
        self:usage()
        return
    end

    for _, item in ipairs(services) do
        local service_name = item.name
        export["service_name"] = service_name
        export["service_id"] = item.service_id
        export["lua_service"] = service_name
        export["debug_port"] = item.debug_port

        -- if service_name == "shop" then
        --     export["lua_service"] = "service"
        -- end

        self:export_to_template(self.template, self.config_name)
        self:start_skynet(self.config_name, service_name, export["service_id"], "log")
    end
    os.execute(string.format("rm %s -f", self.config_name))
end

function MTools:stop(...)
    local args = table.pack(...)
    assert(args[1])
    self:stop_pid(args[1], self.config_name)
end

function MTools:usage()
    print "usage: ./run start all"
    print "       ./run start main"
    print "       ./run start main [-f]         --run front"
    print "       ./run stop shop               --kill service by pid"
    print "       ./run stop all"
end

return MTools
