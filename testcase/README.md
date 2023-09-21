## 已解决问题
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
* C++进程接收pb协议
```sh
在shop_master中接收pb消息，然后使用skynet.redirect重定向到shop服务上，具体如下所示：
skynet.dispatch("lua", function (session , source, sub_type, ...)
    if sub_type == "lua" then
        local args = table.pack(...)
        local cmd = args[1]
        -- local f = assert(CMD[cmd])
        -- local ret = f(table.unpack(args, 2))
        -- if session ~= 0 then
        --     skynet.ret(skynet.pack(ret))
        -- end
    elseif sub_type == "text" then
        local args = table.pack(...)
        local netmsg = args[1]
        if netmsg ~= nil then
            skynet.redirect(shop, source, "text", session, netmsg)
        else
            beelog_error("netmsg is nil")
        end
    end
end)
```
* c++服务往lua服务发送pb协议,使用send方式
```sh
在shop_master进程中，启动本地服务CServiceProxy，用于转发c++发过来的pb协议到其他进程
需要注意：
1.c++服务的消息需要先转发到CServiceProxy，并且需要对消息进行编码，新的消息格式为：【远程节点名长度】【远程服务名长度】【远程节点名】【远程服务名】【原始消息】，其中各部分的长度如下：
  远程节点名长度：占两字节，big endian
  远程服务名长度：占两字节，big endian
  远程节点名：字符串格式，接收消息的节点名称
  远程服务名：字符串格式，接收消息的服务名称，该服务是远程节点上的一个服务
  原始消息：对pb消息进行编码后的消息
  具体实现为Pack.h的OutPackProxy类与serialize_imsg_proxy函数
2.CServiceProxy服务需要将C++服务发过来的消息进行解码，从消息中解出：远程节点名、远程服务名、原始消息；并且将原始消息发到远程节点
```
* c++服务往lua服务发送pb协议,使用rpc_call方式
```sh
要实现rpc_call,需要解决以下两个问题：
1.C++中，在实现rpc_call时会生成一个session，通过session定位回复消息；如果使用skynet_send将这个session该消息发到目的地；
  会出现在socketchannel库中找不到该session,导致错误"socket: unknown session : 32"
2.在dispatcher.lua中会检查session是否为零，如果不为零，且消息处理函数有返回值，则会使用skynet.redirect将回复消息返回到源地址；
  这个实现在cluster模式中行不通，原因是cluster是通过socketchannel进行通信的，使用skynet.redirect发出去的消息无法被解析

解决方案：
针对问题一：我们将应用程序生成的session打包到消息中，并发到目的地，目的进程通过这个session判断是否要回复
针对问题二：我们把【源节点名称】【源服务名称】打包到消息中，在dispatcher.lua中，根据源节点名称、源服务名称发送回复消息
```
* c++服务往c++服务发送pb协议,使用rpc_call方式
```sh
c++服务之间的RPC是通过RpcServer类实现的，具体是在response函数中把回复消息发送到source指定的服务；因此想要将cluster模式应用进来，需要解决以下问题：
1.cluster节点之间直接使用socketchannel通过，无法通过skynet底层的handle，即不能使用source定位到源服务
2.cluster模式中无法使用skynet_send直接发送消息

解决方案：
将【源节点名】【源服务名】【目的节点名】【目的服务名】打包到消息中，将该消息转发到CServiceProxy服务，然后由CServiceProxy服务将消息发往目的节点；
由于CServiceProxy是内部服务，因此可以使用skynet_send发送，消息的类型为PTYPE_TEXT，session固定为1，用于区分send方式的消息
CServiceProxy处理text消息的代码如下所示：
skynet.dispatch("text", function (session, source, netmsg)
    if netmsg ~= nil then
        local remote_node_name, remote_service_name = protopack.unpack_raw_remote_name(netmsg)
        local handle = gtables.get_service_handle(remote_node_name, remote_service_name)
        if handle then
            if session == 0 then
                cluster.send(remote_node_name, handle, "text", netmsg)
            else
                cluster.send(remote_node_name, handle, "rpc_response", netmsg)
            end
        else
            beelog_error("text handler not found remote_node_name:", remote_node_name, "remote_service_name:", remote_service_name)
        end
    end
end)
```

## 待解决的问题
* lua服务往c++服务发送文本消息（skyetext.callc）
* lua服务往c++发送pb消息，通过call_proto方式(参考shopclass.lua)
* c++服务定时器测试
* 是否使用名字服务替换cluster配置，cluster配置不足的地方如下
```sh
1.节点启动后不会主动通知其它节点，而我们的框架中节点之间存在依赖关系，这会导致依赖关系无法实现
2.无法使用配置实时更新节点的热更状态
```

## 其他
* cluster配置更新
```sh
1. 新启动的进程需要加载cluster配置
2. 热更已启动进程的cluster配置
3. 强更集群所有节点的cluster配置
```
* 节点启动后广播通知其它节点,使用名字服务
```sh
1.集群启动时首先启动名字服务
2.集群中的非名字服务节点启动后，往名字服务注册自己；名字服务将该服务启动的消息广播到其他服务
```