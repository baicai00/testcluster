local skynet = require "skynet"
local protopack = require "protopack"
local ltime = require "ltime"

local M = {}

M.__index = M


function M.new(ftable)
	local o = {map = {}, ftable = ftable, last_name = "", yunlog_ctx = nil}
	setmetatable(o, M)
	return o
end

-- 旧接口，可能弃用
function M:register(name, f, obj)
	
	self.map[name] = {f = f, obj = obj}
	---- skynet.error("register self.map = ", tostring(self.map[name]))
end

function M:set_yunlog_ctx(yunlog_ctx)
    --skynet.error("TEST dispatcher set_yunlog_ctx:", tostring(yunlog_ctx))
    local status = nil
	if self.yunlog_ctx then
        if nil ~= self.yunlog_ctx.swtich then
            status = self.yunlog_ctx.swtich
        end
    end
	self.yunlog_ctx = yunlog_ctx
    if nil ~= status then
        self.yunlog_ctx.swtich = status
    end
end

function M:set_yunlog_switch(status)
	if self.yunlog_ctx then
        self.yunlog_ctx.swtich = status
    end
end

function M:_reg(name, obj)
	local funcname
	for i in string.gmatch(name, "%a+") do funcname = i end
	local f = self.ftable[funcname]
	if f then self.map[name] = {f = f, obj = obj} return true, funcname
	else return false, funcname
	end
end

-- 1、注册相应的处理函数
-- 2、无处理函数，报警提示
function M:regs(protobuf, names, obj)
	local client_names = {}
	for _,v in ipairs(names) do
		local ret, funcname = self:_reg(v, obj)
		if not ret then skynet.error("Warning: no func match with ", v) end
		if string.sub(funcname, 1, 1) ~= "i" then table.insert(client_names, v) end
	end
end


function M:dispatch(uid, name, msg, session, source, protobuf)
	---- skynet.error("dispatch self.map = ", tostring(self.map[name]))
	self.last_name = name
	local handler = self.map[name]

	if not handler then
		skynet.error("can't find handler:", name)
		return
	end

	local rsp_pb_name, rsp
	local time_start_us, time_end_us = ltime.microsecond(), 0
	if handler.obj then
		rsp_pb_name, rsp = handler.f(handler.obj, uid, msg, session, source, protobuf)
	else
		-- skynet.error("call uid = ", uid)
		-- skynet.error("call msg = ", tostring(msg))
		rsp_pb_name, rsp = handler.f(uid, msg, session, source, protobuf)
	end
	if rsp_pb_name and rsp and session and session > 0 then
		local rpc_data = protopack.pack_raw(protopack.SUBTYPE_RPC_CLIENT, uid, "pb." .. rsp_pb_name, rsp, protobuf)
		skynet.redirect(source, skynet.self(), 'rpc_response', session, rpc_data)
	end

    --skynet.error("TEST yunlog_ctx:", tostring(self.yunlog_ctx))
	if self.yunlog_ctx and self.yunlog_ctx.log_address and self.yunlog_ctx.protobuf and (nil == self.yunlog_ctx.swtich or self.yunlog_ctx.swtich == true)then
		time_end_us = ltime.microsecond()
		local send_log = {
			service_name = self.yunlog_ctx.service_name,
			harborid = skynet.harbor(skynet.self()),
			pb_name = name,
			time_start_us = time_start_us,
			time_end_us = time_end_us
		}
		--skynet.error("TTT iPostYunLogWorkload", " yunlog_ctx:", tostring(self.yunlog_ctx), " send log:",  tostring(send_log))
		send_log = protopack.pack_raw(protopack.SUBTYPE_SYSTEM, 0, "pb.iPostYunLogWorkload", send_log, self.yunlog_ctx.protobuf)
		skynet.send(self.yunlog_ctx.log_address, 'text', send_log)
	end
end

-- function M:test_send_yunlog(send_log)
--     if not send_log then
--         return
--     end
--     --skynet.error("TEST test_send_yunlog", " yunlog_ctx:", tostring(self.yunlog_ctx), " send log:",  tostring(send_log))
--     send_log = protopack.pack_raw(protopack.SUBTYPE_SYSTEM, 0, "pb.iPostYunLogWorkload", send_log, self.yunlog_ctx.protobuf)
--     skynet.send(self.yunlog_ctx.log_address, 'text', send_log)
-- end

return M
