---
layout: post
title: "浅谈基于Java系统的大型互联网技术"
description: ""
category: 分布式系统
tags: [分布式系统]
---
 
本文旨在简单介绍大型互联网的架构和核心组件实现原理。本文是[笔者](http://weibo.com/geecoodeer)对以前阅读的资料进行阶段性的总结，欢迎大家指正。

解决问题的通用思路是将分而治之（divide-and-conquer），将大问题分为若干个小问题，各个击破。在大型互联网的架构实践中，无一不体现这种思想。

## 大型互联网架构
### 架构目标
商业利益，注意性价比
分而治之，每个组件
 
马车，八匹马   集群，分布式
功能需求
LL 低延迟 见下图的描述，
HA 高可用 ，主要手段是冗余 
base cap 
HR 高可靠

HL 大流量
伸缩性 ：注重线性扩展，是否可以容易通过加入机器来处理不断上升的用户访问压力。
扩展性 ：主要解决功能扩展
### 计算机体系结构
NUMA
 
overhead 
顺序存储： 
随机存储：

### 典型实现
下面典型的一次web交互请求示意图。
![]({{ BASE_PATH }}/images/distributed-system/http request via big internet site.png )

浏览器访问DNS解析，本地如果存在缓存，host主机映射 

区分重复计算（）和动态计算。 静态文件，js，css，图片（大头）； 动态数据
数据变化频率 

### CDN
CDN部署在网络提供商的机房里面。在用户请求网站服务时，可以从距离自己最近的网络提供商获取数据。比如视频网站和内容网站的热点内容。

squid是缓存服务器科班出生
varnish是觉得squid性能不行，纯内存缓存服务器方案
nginx cache是属于不务正业，得益于nginx强大的性能
 
squid 越俎代庖自己实现了一套内存页/磁盘页的管理系统，但这个虚拟内存swap其实linux内核已经可以做得很好，squid的多此一举反而影响了性能

而varnish的内存管理完全交给内核，当缓存内容超过内存阈值时，内核会自动将一部分缓存存入swap中让出内存。以挪威一家报社的经验，1台varnish可以抵6台squid的性能。

nginx cache如知友所说更适合缓存纯文本体积较小的内容，不过如果对nginx框架理解够深，在其上搭建一个山寨varnish不是难事。

淘宝大量商品截图，基本都是缓存在

### RP
反向代理部署在网站的机房内，当用户请求到达中心机房后，首先访问的是反向代理服务器。如果反向代理中缓存者用户请求的资源，就将其直接返回给用户。

网站的静态资源如js，css，图片等资源独立分布式部署，并采用独立的域名。  

缓存整体请求数据；降低应用负载。浏览器并发能力限制。 

nginx   sendfile
### LB
dns 缓存，不能保证及时性。禁用缓存？ 自建cdn还是dns？

nginx+keepalived 。Nginx 提供负载均衡的能力，keepalived 提供健康检查，故障转移，提高系统的可用性。采用这样的架构以后很容易对现
有系统进行扩展，只要在后端添加或者减少realserver，只要更改配置文件，并能实现无缝配置变
更。在本项目中建议采用由Nginx 充当LoadBalancer 的职责，Keepalived 实现Nginx 的HA 的架
构模式，通过暴露虚拟IP（配置DNS 绑定域名）的形式对内容网站进行访问。

负载均衡解决集群问题 
lvs
硬件LB ，监控状态，负载均衡策略，路由转发 

反向代理
DNS 禁用缓存
数据链路层

算法  RR WRR  P105

### WEB APP


使用客户端缓存技术，yahoo 21 ，bigpipe 淘宝首页新技术？浏览器并发数限制，所以会把静态文件分发在不同域名上。
工具 yslow firebug httpwatch css_tidy 
                      
front web app   ：dns，
cdn （varnish，squid）， 小文件 cache系统。 小文件系统。 数据块的大小。 block ，node？？？
                         
restful api， mvc，velocity，jetty/tomcat，bigpipe，taobao 首页？？

 静态请求 不发送到实际生产系统 ；防止恶意数据进行缓存攻击？

 
session   framework  （web开发）

spring 管理对象生命周期。


### SOA
RPC框架，SDL Service Definition Language，服务定义语言，message type，service call，receiving type
功能集合，数据协议和传输协议
ali blog thift，pb， 
stub skelton
传统webservice  
cxf动态生成


netty                    
     
 mq  metaq，hsf，通过监控生产者，broker，消费者之间的队列情况，动态决定增加、减少消费者的


orm mybatis， sql meta data，反射。事务 threadlocal static finally remove
cache redis，江南白衣兄的redis 分享。 缓存集群？ 还是善用操作系统缓存。
solar 分布式搜索，中文分词库 

bi，机器学习分析用户行为，结合搜索进行推荐排名。 Hadoop，storm

运维 os （cpu，memory，disk（空间，读写次数）） 中间件 tomcat，jetty，jvm，network，

能源消耗 （制冷，电能，水冷） 地震带。 自动运维

存储    db,nosql,hbase, 分区表，字段压缩，查询优化，awr报告。

功能特点
核心原理
重要算法 


### MQ
提升网站响应速度 
消除并发访问洪峰
 
异步回调功能
Kafka


### CACHE
存在热点数据，某个时间段内有效。
一致性Hash P109
LRU
缓存预热  
REDIS
### STORAGE
ssd vs 机械硬盘
ACID，Atomic，Consistent，Isolated，Durable

redo undo 定期刷新
存储 分为 大文件，小文件，结构化，非结构化，分布式文件存储和数据库存储。数据库存储 读写分离，垂直扩展（scale up） 分表，水平扩展（scale out）分库， 

分布式文件系统

数据一致性 级别

比RAID好多了”– 在一个非存储区域网络的RAID（non-SAN RAID）的建立中，磁盘是冗余的，但主机不是，如果你整个机器坏了，那么文件也将不能访问。 MogileFS分布式文件存储系统在不同的机器之间进行文件复制，因此文件始终是可用的。 主要是无法解决网络分区的问题。

### 配置管理
zk diamond

### SEARCH
搜索 分为 排序，推荐，BI,大数据
spark，storm 实时流处理

### 其他
安全分析 
安全，虚拟化 是贯穿全系统的 

开发，发布，监控，预警，运维

Agent —》 Explorer ，Analyze，Visual，Dashboard，Share，
 dapper  日志规范化 
 网易和weibo收藏
 failover
matrix  ，各种延迟，各种指标
collection
mornitor ， 图表，找到根因，重建索引，rebuild index
定期扫描应用日志，针对错误 进行告警 

开发环境，测试环境，灰色发布，回滚 流程，协调。 计划，特性开发。
SCR source code responsitoy   

package  编译 obj 独立部署
 
project 
          target
          dependency
                      version number
          build dependency


Deploy Environment 支持开速发布，不用大的cycle，灰色发布 ，通过LB来控制，先不发送请求 。根据系统使用率动态启动，停止实例。
         Host

云厂商 硬件申购， 
     因为：固定资产开销，可能闲置， 购买，管理，安装费用 ，无法迅速购买
     活动消费，类似开车和租车的区别 
     能源，制冷，运维成本，量大硬件定制
     充分利用闲置资源
数据中心
 
## 分布式系统原理
服务框架自省（运维监控） 依赖关系统计，前台系统访问路径，自动、手工降级，系统问题自动排查甚至问题自动修复，时长，队列情况。


瘦客户端 还是胖客户端 瘦的避免频繁升级


备份和低一致性 来提升可用性和网络分区容忍性
hostclass —》 host
fault torlerent
每个消息存储3个节点 
at least once，重发+唯一主键策略。
故障检测，Faliure  Detection

数据压缩 批处理
亚马逊中文论文 ，中间件ali blog

集群  一群机器服务于同一个功能， 分布式 则是一群机器服务于不同的功能
 
C 所有节点看到一致的数据 ，3个副本由2个写成功即可。 All or Nothing，or partcial???
A 
P 分布式算法，2pc 两阶段提交 mvcc

缓存穿透
         
一致性hash 用于缓存和数据分区。 但是后者join 比较困难。 维护成本？

### 优秀设计思想
 dubbo作者
faliure detection 
 通过增加一层来解决问题。包括日志的mvc模型，日常的模块解耦，map-reduce模型，
各种事件，启动，停止，

数据中心，自动化代码管理，测试，安全检查
  
移位操作 OP_ACCEPT,OP_READ。。。
 
 通过负载均衡来开启新机器的路由 
数据预处理。
thread local 隐式传参，线程安全

HLR 位置营销 救援 
 ring buffer
  
度量工具，静态检查，安全扫描工具
 coc 约定优于配置 

springside的推荐
  
string hashcode —》 cache
hashmap —》 2的n次方
valueOf
 
core+plugin(ConfigFile/参数设置+ Interpter Filter Pattern+ Observer Pattern)+eat dogfood

paxos 扩展协议

心跳检查
 
各种关键字解释 

何谓架构，很多是一家之言。
 
根据业务划分多个子系统 
  
用户用量计费  读写，网络消耗，  发送  中心，计算账单 数据服务  
     
连接恢复，还是新开连接？
   
8-10个人，负责一两个service
领导的需求             

根据业务区分不同的业务部门  业务架构 
     
TDDL ， sql proxy、agent，binlog sync 分区分表 
  
白马非马，架构是什么？内涵 外延 。归纳和演绎。 不完全归纳。 混淆内涵和外延。
 
log4j 只有一个log组件，通过 find xar jar ?? 见csdn 收藏。
## 互联网常用算法


二分
快排
B+树
前缀索引
布隆算法
剪枝算法
radix tree 
倒排索引
trie common prefix search
纠错算法，垃圾邮件算法
压缩
连接泄露原理是查询session视图，获得每个session的创建时间，看看那些session生命周期过长。然后找到对应执行的sql语句和业务代码。
 
写出脉络，框架，再去搞细节。
 
把发生故障当成常态来设计
配置统一管理
 
dsl

对象状态

open api
 
robbin 的cache 设计
 
bean tostring
框架设计原则和范式


通用系统
特色系统


尽量避免依赖第三方jar
生态系统建设
* 内部
* 外部
 
支持扩展点，插件 
  
## 参考


* 《大规模分布式存储系统原理解析与架构实战》
* 《大型网站技术架构核心原理与案例分析》
* 《分布式系统原理介绍》
* 《大规模Web服务开发技术》
* 《程序员》 2014 第一期
*  [varnish / squid / nginx cache 有什么不同？](http://www.zhihu.com/question/20143441)



AUPE

内核 vs 库函数   sbrk vs malloc
creat 是 open 的简便函数，封装了open
lseek 没有发生实际的io操作
进程终止时，内核会自动释放句柄
read ahead 、write behind  P71
sync 加入写队列缓存就返回，fsync 写入到磁盘后才返回
为了打开文件，需要具有文件所在目录的执行权限
void *

I/O多路复用（I/O multiplexing）首先构造一张有关描述符的列表，然后调用一个函数，直到这些描述符中的一个已经准备I/O时，该函数才返回。在返回时，它告诉进程哪些描述符已经准备好可以进行I/O

poll、pselect和select这3个函数使我们能够执行I/O多路复用。

p581 


---

网络编程

电话类比网络连接关系原来在UP中已经阐述。

TIME_WAIT 允许最大跳数内的并且小于2MSL的数据包重传

P36 3次握手状态

P78

p85  baklog 图

listen 指定 backlog。内核维护两个队列 未完成连接队列（处于SYN_RCVD状态），已完成连接队列

毒药 和哨兵 dummy

syn flood 攻击  忽略了大量关于c语言之间的描述

accept 监听套接字 和  已连接套接字  前者一直存在  后者用完则关闭

p92

p122 必读 nio

select 的最大描述符设置需要重新编译或者使用新版本的的参数设置；线性搜索，扩展能力差。
选择 epoll

select 可读，可写，异常
无限等待，等待某段时间，不等待

p361 解释了 为什么需要finishConnect

僵尸进程，处理signal 。sigal handler



一步步解释为什么操作系统会这么复杂
基本的输入，输出模型
性价比 
为了提升性能而做的各种tradeoff
引入一种模型后，然后又存在哪些问题
虚拟内存，tlb，cpu，磁盘




一步步解释为什么操作系统会这么复杂
基本的输入，输出模型
性价比 
为了提升性能而做的各种tradeoff
引入一种模型后，然后又存在哪些问题
虚拟内存，tlb，cpu，磁盘


dubbo 怎么使用Javaassist的


AtomicIntegerFieldUpdater

infoq 2个人 文章复习 我的blog文章复习
网易那篇多线程复习 

整体浏览我的日志 

CSAPP 

补码是非对称编码
补码计算 

P43 介绍 反码和原码
 
抓住主要矛盾

性价比原理 寄存器 缓存  内存 磁盘
局部性原理（时间局部性 和空间局部性）：在具备良好时间局部性的程序中，被引用一次的存储器位置很可能在不远的将来会被再次引用。在具备良好空间局部性的程序中，被引用一次的存储器位置的附近位置很可能在不远的将来会被再次引用。

 cach miss （矩阵计算，二维数组本质上是一维数组，双重循环），false sharing
并发性原理 取指(内存到寄存器) 译码（） 计算  访存  写回  更新pc  流水线  乱序执行  分支预测(排序执行)
虚拟内存原理  
tlb  方便管理内存，避免重复IO 虚拟内存 支持
	 内存容量不够 swap机制
扫一扫+工具检测 
降低循环内的低效计算
数组和链表的区别
并发锁的粒度和策略

cpu --存储控制器  -- dram -- 先将行数据复制到内存的内部缓冲区内，然后再根据列去取数据

地址，数据 内存 

旋转磁盘和固态硬盘

磁盘 ：512字节 300扇区 20000磁道  2盘面 5盘片 

寻道时间 旋转时间 传送时间 前2者耗时较大，访问第一个自己耗费很长时间，后面几乎不耗费时间。所以操作系统会预读。。

逻辑磁盘块：现代磁盘构造复杂，为了对os隐藏这些复杂性，现代磁盘提供了一个简单的视图，称为逻辑块。磁盘控制器维护着逻辑块和实际物理的磁盘扇区之间的映射关系。os执行i/o操作时，通过制定逻辑块号，磁盘控制器的固件执行一个快速查找，将逻辑号翻译成（盘面，磁道，删除）的3元组，这个3元组唯一标识了对应的物理扇区。控制器的硬件解释这个3元组，将读、写头移动到适当的柱面，等待扇区移动到读写头下，将读写头感知的位放到控制器里面的一个小缓冲区内，然后复制到内存中。


虽然xxx详细描述超出了我们的讨论反问，但是我们可以概要描述下。


存储器映射I/O: cpu使用这种技术来想i/o设备发送命令。首先在唉地址空间中。有一块地址是为与I/O设备通信保留的。每个这样的地址称为一个I/O port。当一个I/O设备连接到总线时，它与一个或多个端口相关联。

cpu可能通过执行3个对 I/O port （比如为0Xa0）的存储指令。当磁盘控制器收到cpu读命令后，它将逻辑块翻译成扇区地址 ，然后将内容直接传送到主存，不需要cpu的干涉。这种过程称为DMA。


{% include JB/setup %}
