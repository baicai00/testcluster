local skynet = require "skynet"
require "skynet.manager"
local mysql = require "mysql"
local memcache = require "memcache"
local gtables = {}   -- hotfix table

local CMD = {}
local pool = {}
local pool_reconn_lock = {}
local pool_reconn_pending = {}

local maxconn
local index = 1
local host

function gtables.getconn(channel)
    local now_index = 0 
    channel = tonumber(channel) or 0
    local db
    if channel ~= 0 then
        now_index = channel % maxconn + 1
        db = pool[channel % maxconn + 1]
    else
        now_index = index
        db = pool[index]
        index = index + 1
        if index > maxconn then
            index = 1
        end
    end
    assert(db)
    return db, now_index
end


function gtables.reconn(db, now_index)
    if not host then return false end
    db.kk_is_active = false
    if pool_reconn_lock[now_index] then
        local pending = pool_reconn_pending[now_index]
        if pending:size() > 2000 then
            beelog_info("too many to pending")
            return false
        end
        local thread = coroutine.running()
        pending:push_back(thread)
        skynet.wait()
    end
    db = pool[now_index]
    if db.kk_is_active then
        local pending = pool_reconn_pending[now_index]
        if pending:size() > 0 then
            local thread = pending:pop()
            skynet.wakeup(thread)
        end
        return true
    end

    pool_reconn_lock[now_index] = true

    local unlock_func = function()
        pool_reconn_lock[now_index] = false
        local pending = pool_reconn_pending[now_index]
        if pending:size() > 0 then
            local thread = pending:pop()
            skynet.wakeup(thread)
        end
    end

    local to_connet_func = function()
        local new_db = mysql.connect{
            host =   host[1],
            port = tonumber(host[2]),
            database = skynet.getenv("mysql_database"),
            user = skynet.getenv("mysql_user"),
            password = skynet.getenv("mysql_password"),
            max_packet_size = 1024 * 1024
        }

        if new_db then
            pcall(db.disconnect, db)
            new_db:query("set charset utf8mb4")
            new_db.kk_is_active = true
            pool[now_index] = new_db
            beelog_info("TTT reconn success", now_index)
            unlock_func()
            return true
        else
            beelog_error("mysql reconn error")

            unlock_func()
            return false
        end
    end

    local status, retval = pcall(to_connet_func)
    if status then
        return retval
    else
        unlock_func()
        return false
    end
end

function CMD.start(service_name)
    if service_name then
        skynet.register(service_name)
    end
    maxconn = tonumber(skynet.getenv("mysql_maxconn")) or 10
    assert(maxconn >= 2)
    -- beelog_info("host = " , skynet.getenv("mysql_host"))
    host = string.split(skynet.getenv("mysql_host"), ":")
    -- beelog_info("ip = ", host[1])
    -- beelog_info("port = ", host[2])
    for i = 1, maxconn do
        local db = mysql.connect{
            host =   host[1],
            port = tonumber(host[2]),
            database = skynet.getenv("mysql_database"),
            user = skynet.getenv("mysql_user"),
            password = skynet.getenv("mysql_password"),
            max_packet_size = 1024 * 1024
        }

        

        if db then
            table.insert(pool, db)
            db:query("set charset utf8mb4")
            pool_reconn_lock[i] = false
            pool_reconn_pending[i] = memcache:new()
        else
            beelog_error("mysql connect error")
            return false
        end
    end
    return true
end

function CMD.execute(sql, channel)
    -- beelog_info("execute", channel)
    local db, now_index = gtables.getconn(channel)
    local status, err = pcall(db.query,db,sql)
    if status then
        return err
    else
        beelog_info(" to reconn ", now_index, tostring(err))
        local code, retval = pcall(gtables.reconn, db, now_index)
        if code and retval then
            db = pool[now_index]
            status, err = pcall(db.query,db,sql)
            if status then
                return err
            else
                beelog_error("mysql execute error after reconn", err, sql)
                return {}
            end
        else
            beelog_error("mysql execute error", err, sql)
            return {}
        end

    end
end


function CMD.execute_send(sql, channel)
    -- beelog_info("execute_send", channel)
    local db, now_index = gtables.getconn(channel)
    local status, err = pcall(db.query,db,sql)
    if status then

    else
        beelog_info(" to reconn ", now_index, tostring(err))
        local code, retval = pcall(gtables.reconn, db, now_index)
        if code and retval then
            db = pool[now_index]
            status, err = pcall(db.query,db,sql)
            if status then

            else
                beelog_error("mysql execute_send error after reconn", err, sql)
            end
        else
            beelog_error("mysql execute_send error", err, sql)
        end

    end

    return nil
end

-- 过滤SQL注入字符
function CMD.quote_sql_str(str)
    local db = gtables.getconn()
    return db.quote_sql_str(str)
end

function CMD.stop()
    for _, db in pairs(pool) do
        db:disconnect()
    end
    pool = {}
    return true
end

function gtables.skynet_start()
    -- skynet.dispatch("lua", function(session, source, cmd, ...)
    --     local mark = gtables
    --     local f = assert(CMD[cmd], cmd .. "not found")
    --     local ret = f(...)
    --     if ret ~= nil then
    --         skynet.retpack(ret)
    --     end
    -- end)

    skynet.dispatch("lua", function (session , source, sub_type, ...)
        beelog_info("sub_type:", sub_type)
        if sub_type == "lua" then
            local args = table.pack(...)
            local cmd = args[1]
            local f = assert(CMD[cmd], cmd .. "not found")
            local ret = f(table.unpack(args, 2))
            if ret ~= nil then
                skynet.retpack(ret)
            end
        else
            assert(false)
        end
    end)

    beelog_info("mysql pool skynet_start SERVICE_NAME:", SERVICE_NAME)
    --skynet.register(SERVICE_NAME)
end

skynet.start(gtables.skynet_start)
