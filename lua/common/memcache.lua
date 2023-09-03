--
-- Date: 12/8/18
-- Time: 11:46 AM
-- 提供简单的O(1)缓存机制，没有校验数据是否重复
--

local memcache = class("memcache")


function memcache:ctor()
    self._tail = 0
    self._head = 0
    self._cache = {}
end

function memcache:init(objs)
    self:ctor()
    for _, v in pairs(objs) do 
        table.insert(self._cache, v)
        self._head = 1
        self._tail = self._tail + 1
    end
end

function memcache:pop()
    local obj = self._cache[self._head]
    if obj ~= nil then
        self._cache[self._head] = nil
        self._head = self._head + 1
    end
    return obj
end

function memcache:pop_back()
    local obj = self._cache[self._tail]
    if obj ~= nil then
        self._cache[self._tail] = nil
        self._tail = self._tail - 1
    else
        return
    end
    return obj
end

function memcache:push_back(obj)
    if obj == nil then return false end
    if self._head == 0 then self._head = 1 end

    self._tail = self._tail + 1
    self._cache[self._tail] = obj
    return true
end

function memcache:size()
    if self._tail == 0 then return 0 end

    return self._tail - self._head + 1
end

function memcache:shuffle()

end

return memcache
