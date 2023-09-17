local skynet = require "skynet"
local cluster = require "bee_cluster"
local protopack = require "protopack_cluster"

local M = {}

M.__index = M

function M.new(ftable)
    local o = {map = {}, ftable = ftable, last_name = "", yunlog_ctx = nil}
    setmetatable(o, M)
    return o
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

function M:dispatch(uid, name, msg)
	---- skynet.error("dispatch self.map = ", tostring(self.map[name]))
	self.last_name = name
	local handler = self.map[name]

	if not handler then
		skynet.error("can't find handler:", name)
		return
	end

	local rsp_pb_name, rsp
	if handler.obj then
		rsp_pb_name, rsp = handler.f(handler.obj, uid, msg)
	else
		rsp_pb_name, rsp = handler.f(uid, msg)
	end
	return rsp_pb_name, rsp
end

return M