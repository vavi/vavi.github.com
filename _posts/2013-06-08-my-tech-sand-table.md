---
layout: post
title: "脑海中的一些技术以及书籍推荐"
description: ""
category: 
tags: []
---

# 未完待续 



# 计算机基础
* K&R c 语言 第二版 
* <<编译原理>>龙书
  *php 编译成 cpp facebook  
* CSAPP
* 算法导论( isak 搜下 有答案版本的 )
* 编程珠玑

# JAVA应用
* classloader 

* Unsafe 
* GC
  * 引用队列 WeakReference.get()  
* ServiceLoader
* thread  
  * JAVA 并发编程实践,并发编程网站  
  * 生产者/消费者
  * 信号量 几种io 
  * wenshao 的线程 
  * double check lock 无用了 
  * BlockingQueue
  * ThreadLocal
* io 
  * 大文件读写
  * nio pdf  
* socket 
  * wireshark
  * socket  pdf 
  * http 
  * tcp 
  * spec 
* collection 
  * 容器类源码研究
  * hash 冲撞 HashMap 死循环  
  * CopyOnWriteArrayList 
* 序列化 
  * pdf
* 语法糖
  * 注解
  * 泛型 



# JAVA基础
 jdk source 
锁优化 锁粗化 
对象逃逸 
有空读读javac源码,编译jdk
String jvm整个过程加载
effective java
blue davey 的ppt(blog和slide share)
     Java Performance     itpub 上面可读 英文原版 
     <<深入jvm>>,<<jvm specification>><<高级编译器设计与实现>>
   java Puzzles 
java.lang.Instrument
jmm内存模型保证引用替换后的原子操作?
btrace
jvm 字符串填充率 回头看下 

# 优秀设计思想
* Clean Code
* 敏捷原则 模式
* 重构
* 设计模式
* 重构与设计模式
* 领域驱动设计
  *  委托 代理 充血/贫血 
* dubbo 作者blog
* 分布式设计模式
* 元数据 配置数据 业务数据 
     
# 典型开源框架
* spring
* netty
* dubbo
  * 实现 remoting-doll
  * rmi
  * 数据协议和传输协议
* mq
  * 实现简单mq,包括简单日志监控,分布式事务(2pc,和参照zhh2009实现机制),参考metaq(diniess git fork 和 2.0版本)和notify,传输输数据格式和传输协议,以及数据暂存(断电考虑),性能测试
* velocity
* tomcat
  * how tomcat works 
* struts
* apache 典型的其他开源工具(Apache Lang Utils) 
* jetty
* drools
* 阿里系的其他开源 (fastjson，druidDatasource(怎么控制数据库连接泄漏？ 代理机制)，tddl) 
* hessian|thift|protobuf|**
* activiti|jbpm


# 典型问题解决方案

* J2EE系统性能优化
  * btrace agapple
  * MAT
  * bigpipe 
  * 淘宝和腾讯优化思路 PPT 官网分析  
* session/cookie
* 日志分析
  *  hadoop
* 分布式系统管理
  * zookeeper
* 分布式事务
  * 两阶段式提交
  * lealone
  * cmt事务
* 事务补偿
  * 	
* JMX管理业务参数
* 架构演进
  * ROR
  * 垂直 水平  分布式
* 文件同步与备份
  * rsyn
* 系统安全
  * 不熟… 自我提升,官方补丁,工具检测,知名安全论坛,wooyun,搞好安全圈的人脉
* 防止重复提交
* 乱码问题
* 写个cpu和io 100%忙的代码   

# 操作系统工具及应用

Mou
EverNote
PDF
Eclipse
QQ
Chrome

基本命令,awk,grep,sed



# 操作系统实现原理
    linux 内核 (虚拟化 
  
# 数据库应用

# 数据库实现
 高性能MySQL
 hbase
 
# 硬件相关
* CPU
* Memory
* Disk
* NIC
* optical fiber
* logical circuit 
* 虚拟化  
 
# 前端UI
html
css 
oo js
jQeury,kissy,seajs
浏览器实现原理(coolshell 和 ?winter)
ror 有几个很优秀的前端视频
图片预占位
bigpipe
复杂页面 


# 如何设计高可用,高扩展,高性能的网站
CAP/ECAP
谷歌论文(crubby,gfs...)

首先从common需求角度
行业特殊需求

使用开源组件
	taobao,业界开源
	日本那本书
代码级别
	可维护
	算法
机器学习
搜索算法
成本
AI

图片 视频 游戏 GIS 
，语音/图像识别， 
 
 
# 其他 
  
以前看的书(公司图书馆,自己以前的电子书) 
  
   

{% include JB/setup %}
