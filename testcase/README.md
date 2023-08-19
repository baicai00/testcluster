## TODO LIST
* 基于skynet v1.6.0版本升级当前框架的cluster模块，具体如下：
```sh
1. 当前框架是指KKT使用的skynet版本
2. 不改变当前框架的skynet模块，而是将v1.6.0的cluster模块作为业务框架的一部分引入，各服务的命名加上“bee_”前缀，例如clusterd服务的名字改为bee_clusterd
3. bee_clusteragent服务的dispatch_request函数有改动，去掉了tracetag与skynet.ignoreret函数的调用；原因是KKT框架不支持
```
* 不同进程之间发送PB协议 (todo)
* 配置更新 （todo）
```sh
1. 新启动的进程需要加载cluster配置
2. 热更已启动进程的cluster配置
3. 强更集群所有节点的cluster配置
```
* 我热更后如何改变其它进程中缓存的我的信息（todo）


## cluster集群模式热更流程 （todo）