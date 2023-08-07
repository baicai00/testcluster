local tconcat = table.concat
local tinsert = table.insert
local srep = string.rep

-- lua扩展

-- table扩展

-- 返回table大小
table.size = function(t)
    local count = 0
    for _ in pairs(t) do
        count = count + 1
    end
    return count
end

-- 判断table是否为空
table.empty = function(t)
    return not next(t)
end

-- 返回table索引列表
table.indices = function(t)
    local result = {}
    for k, v in pairs(t) do
        table.insert(result, k)
    end
    return result
end

-- 返回table值列表
table.values = function(t)
    local result = {}
    for k, v in pairs(t) do
        table.insert(result, v)
    end
    return result
end

-- 浅拷贝
table.clone = function(t, nometa)
    local result = {}
    if not nometa then
        setmetatable(result, getmetatable(t))
    end
    for k, v in pairs (t) do
        result[k] = v
    end
    return result
end

-- 深拷贝
table.copy = function(object)
    local lookup_table = {}
    local function _copy(object)
        if type(object) ~= "table" then
            return object
        elseif lookup_table[object] then
            return lookup_table[object]
        end
        local new_table = {}
        lookup_table[object] = new_table
        for key, value in pairs(object) do
            new_table[_copy(key)] = _copy(value)
        end
        return setmetatable(new_table, getmetatable(object))
    end
    return _copy(object)
end

table.merge = function(dest, src)
    for k, v in pairs(src) do
        dest[k] = v
    end
end

-- string扩展

-- 下标运算
do
    local mt = getmetatable("")
    local _index = mt.__index

    mt.__index = function (s, ...)
        local k = ...
        if "number" == type(k) then
            return _index.sub(s, k, k)
        else
            return _index[k]
        end
    end
end

string.split = function(s, delim)
    local split = {}
    local pattern = "[^" .. delim .. "]+"
    string.gsub(s, pattern, function(v) table.insert(split, v) end)
    return split
end

string.ltrim = function(s, c)
    local pattern = "^" .. (c or "%s") .. "+"
    return (string.gsub(s, pattern, ""))
end

string.rtrim = function(s, c)
    local pattern = (c or "%s") .. "+" .. "$"
    return (string.gsub(s, pattern, ""))
end

string.trim = function(s, c)
    return string.rtrim(string.ltrim(s, c), c)
end

local function ori_dump(obj)
    local getIndent, quoteStr, wrapKey, wrapVal, dumpObj
    getIndent = function(level)
        return string.rep("\t", level)
    end
    quoteStr = function(str)
        return '"' .. string.gsub(str, '"', '\\"') .. '"'
    end
    wrapKey = function(val)
        if type(val) == "number" then
            return "[" .. val .. "]"
        elseif type(val) == "string" then
            return "[" .. quoteStr(val) .. "]"
        else
            return "[" .. tostring(val) .. "]"
        end
    end
    wrapVal = function(val, level)
        if type(val) == "table" then
            return dumpObj(val, level)
        elseif type(val) == "number" then
            return val
        elseif type(val) == "string" then
            return quoteStr(val)
        else
            return tostring(val)
        end
    end
    dumpObj = function(obj, level)
        if type(obj) ~= "table" then
            return wrapVal(obj)
        end
        level = level + 1
        local tokens = {}
        tokens[#tokens + 1] = "{"
        for k, v in pairs(obj) do
            tokens[#tokens + 1] = getIndent(level) .. wrapKey(k) .. " = " .. wrapVal(v, level) .. ","
        end
        tokens[#tokens + 1] = getIndent(level - 1) .. "}"
        return table.concat(tokens, "\n")
    end
    return dumpObj(obj, 0)
end

local function dump(root)
    local cache = {  [root] = "." }
    local function _dump(t,space,name)
        local temp = {}
        for k,v in pairs(t) do
            local key = tostring(k)
            if cache[v] then
                tinsert(temp," +" .. key .. " {" .. cache[v].."}")
            elseif type(v) == "table" then
                local new_key = name .. "." .. key
                cache[v] = new_key
                tinsert(temp," +" .. key .. _dump(v,space .. (next(t,k) and " |" or " " ).. srep(" ",#key),new_key))
            else
                tinsert(temp," " .. key .. " [" .. tostring(v).."]")
            end
        end
        return tconcat(temp,"\n"..space)
    end
    return "\n" .. _dump(root, "","")
end

do
    local _tostring = tostring
    tostring = function(v)
        if type(v) == 'table' then
            return dump(v)
        else
            return _tostring(v)
        end
    end
end

-- math扩展
do
    local _floor = math.floor
    math.floor = function(n, p)
        if p and p ~= 0 then
            local e = 10 ^ p
            return _floor(n * e) / e
        else
            return _floor(n)
        end
    end
end

math.round = function(n, p)
        local e = 10 ^ (p or 0)
        return math.floor(n * e + 0.5) / e
end


-- lua面向对象扩展
function class(classname, super)
    local superType = type(super)
    local cls

    if superType ~= "function" and superType ~= "table" then
        superType = nil
        super = nil
    end

    if superType == "function" or (super and super.__ctype == 1) then
        -- inherited from native C++ Object
        cls = {}

        if superType == "table" then
            -- copy fields from super
            for k,v in pairs(super) do cls[k] = v end
            cls.__create = super.__create
            cls.super    = super
        else
            cls.__create = super
            cls.ctor = function() end
        end

        cls.__cname = classname
        cls.__ctype = 1

        function cls.new(...)
            local instance = cls.__create(...)
            -- copy fields from class to native object
            for k,v in pairs(cls) do instance[k] = v end
            instance.class = cls
            instance:ctor(...)
            return instance
        end

    else
        -- inherited from Lua Object
        if super then
            cls = {}
            setmetatable(cls, {__index = super})
            cls.super = super
        else
            cls = {ctor = function() end}
        end

        cls.__cname = classname
        cls.__ctype = 2 -- lua
        cls.__index = cls

        function cls.new(...)
            local instance = setmetatable({}, cls)
            instance.class = cls
            instance:ctor(...)
            return instance
        end
    end

    return cls
end

function iskindof(obj, classname)
    local t = type(obj)
    local mt
    if t == "table" then
        mt = getmetatable(obj)
    elseif t == "userdata" then
        mt = tolua.getpeer(obj)
    end

    while mt do
        if mt.__cname == classname then
            return true
        end
        mt = mt.super
    end

    return false
end


function print_r(root)
    local cache = {  [root] = "." }
    local function _dump(t,space,name)
        local temp = {}
        for k,v in pairs(t) do
            local key = tostring(k)
            if cache[v] then
                table.insert(temp,"+" .. key .. " {" .. cache[v].."}")
            elseif type(v) == "table" then
                local new_key = name .. "." .. key
                cache[v] = new_key
                table.insert(temp,"+" .. key .. _dump(v,space .. (next(t,k) and "|" or " " ).. string.rep(" ",#key),new_key))
            else
                table.insert(temp,"+" .. key .. " [" .. tostring(v).."]")
            end
        end
        return table.concat(temp,"\n"..space)
    end
    print(_dump(root, "",""))
end


local _class={}

function class2(super)
    local class_type={}
    class_type.ctor=false
    class_type.super=super
    class_type.new=function(...)
        local obj={}
        do
            local create
            create = function(c,...)
                if c.super then
                    create(c.super,...)
                end
                if c.ctor then
                    c.ctor(obj,...)
                end
            end

            create(class_type,...)
        end
        setmetatable(obj,{ __index=_class[class_type] })
        return obj
    end
    local vtbl={}
    _class[class_type]=vtbl

    setmetatable(class_type,{__newindex=
    function(t,k,v)
        vtbl[k]=v
    end
    })

    if super then
        setmetatable(vtbl,{__index=
        function(t,k)
            local ret=_class[super][k]
            vtbl[k]=ret
            return ret
        end
        })
    end

    return class_type
end