## TODO LIST
* Lux项目使用的cluster模块版本太旧
```sh
基于skynet v1.6.0版本升级当前框架的cluster模块，具体如下
1. 不改变当前框架的skynet模块，而是将v1.6.0的cluster模块作为业务框架的一部分引入，各服务的命名加上“bee_”前缀，例如clusterd服务的名字改为bee_clusterd
2. bee_clusteragent服务的dispatch_request函数有改动，去掉了tracetag与skynet.ignoreret函数的调用；原因是KKT框架不支持
3. 在bee_clusteragent中使用到了skynet.rawsend，该函数是后面版本增加，需要在skynet.lua中增加skynet.rawsend函数，具体分别下：
function skynet.rawsend(addr, typename, msg, sz)
    local p = proto[typename]
    return c.send(addr, p.id, 0 , msg, sz)
end
```
* Lua进程之间发送PB协议
```sh
#在lobby进程中，使用cluster.query查询“lpersistence_3”进程中的"lpersistence"服务的地址
local status, lpersistence = pcall(cluster.query, "lpersistence_3", "lpersistence")

#在lobby进程中，使用cluster.send发送PB协议，消息的类型为"text",具体如下：
cluster.send("lpersistence_3", lpersistence, "text", pack_msg)

#在lobby进程中，使用cluster.call发送lua消息，消息的类型为"lua",具体如下：
local ret = cluster.call("lpersistence_3", lpersistence, "lua", "ping")
beelog_info(ret)

#在cluster内部传递消息时，消息的类型仍是"lua"，因此在lpersistence_3进程中，需要对skynet.dispatch做一些修改，具体如下：
skynet.dispatch("lua", function (session , source, sub_type, ...)
    if sub_type == "lua" then
        local args = table.pack(...)
        local cmd = args[1]
        local f = assert(CMD[cmd])
        local ret = f(table.unpack(args, 2))
        if session ~= 0 then
            skynet.ret(skynet.pack(ret))
        end
    elseif sub_type == "text" then
        local args = table.pack(...)
        local netmsg = args[1]
        if netmsg ~= nil then
            local uid, name, msg = protopack.unpack_raw(netmsg, protobuf)
            dispatcher:dispatch(uid, name, msg, session, source, protobuf)
        end
    end
end)
```
* lobby给lpersistence的子服务mysqlpool发送lua消息
(1) 直接向mysqlpool发送消息，这必须要在mysqlpool中修改skynet.dispatch的实现
```sh
# 在lobby进程中，使用cluster.query查询“lpersistence_3”进程中的"mysqlpool"服务的地址
local status, mysqlpool = pcall(cluster.query, "lpersistence_3", "mysqlpool")

# 在lobby进程中，使用cluster.send发送lua消息,具体如下：
local sql = "select * from pppoker.user where uid = 10076447"
local ret = cluster.call("lpersistence_3", mysqlpool, "lua", "execute", sql)
beelog_info(tostring(ret))
```
(2) 通过activity_master主服务转发消息到子服务
```sh
# 在activity_mater主服务上启动8个ativity子服务，转发消息时通过对uid进行哈希运行，以确定处理消息的子服务
```
* C++进程发送与接收pb协议（todo）
```sh
存在的问题：
1. c++进程直接使用了skynet_send发送消息，而该API依赖于harbor
2. cluster相关的服务都是Lua服务，而且cluster发过来的消息必须由clusteragent服务接收，c++进程没有启动cluster相关的服务
```
* 配置更新 （todo）
```sh
1. 新启动的进程需要加载cluster配置
2. 热更已启动进程的cluster配置
3. 强更集群所有节点的cluster配置
```
* 我热更后如何改变其它进程中缓存的我的信息（todo）


## cluster集群模式热更流程 （todo）