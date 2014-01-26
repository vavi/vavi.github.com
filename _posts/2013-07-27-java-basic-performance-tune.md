---
layout: post
title: "记几次JAVA系统故障问题定位过程"
description: ""
category: 宕机及优化
tags: [宕机及优化]
---
把自己以前碰到的case汇总列下，作为对自己过去的一部分工作总结。
## 问题定位一般步骤
1. 具备常见的理论知识，不一定要全记住细节。但是需要知道问题的关联性，然后根据某些关键字搜索或者查阅资料等。
2. 沟通故障现象，根据故障的严重性决定是重启规避还是在现场直接定位。是集群，单机，某个业务系统，抑或某个业务模块发生问题。
3. 查看系统运行情况，比如应用日志，jvm内存，线程，操作系统的cpu，内存，磁盘等情况。
4. 针对性分析，这个只能具体问题具体分析。 

---

## 案例分享
### 内存泄露
#### 现象和原因
这类故障的典型现象是系统无响应，功能基本全部瘫痪。常见原因包括：

* 查询数据库结果集过大，没有进行分页处理。
* 过大的缓存对象占用了大量的内存空间
* 或者是频繁创建了大对象，导致JVM新生代和老年代处于100%，系统频繁FullGC。

#### 定位方法
定位这一类问题的神器是使用[MAT](http://www.eclipse.org/mat/downloads.php)工具。
介绍MAT的文章很多了，入门的可以看看[这篇](http://tivan.iteye.com/blog/1487855)。有几个小细节，需要说下：

* 最容易被忽略的就是忘记打开“keep unreachable objects” 参数了。有时候，dump下来的文件很大，但是mat分析时，却显示整体占用内存很小时，就需要打开这个参数。
* 有时候幸运的话，能够根据方法链或者类名称就能直接看到哪些对象占用过大。如果碰到map，则需要看到里面的属性，比如根据map的key值在代码中全文搜索。
* 有时候，jmap出现bug或者无法直接通过jmap命令来导出dump时，可以使用gcore <pid>命令。
* 有时kill -3没有设置好或者jstack 出现bug时，也可以在mat里面查看线程堆栈情况，只是看起来比较痛苦。
* 在micro benchmark时，可以查看下容器的填充率。 
* 当然，既然需要分析内存占用情况，你有时候肯定想看看java对象大小究竟是怎么计算的。这个后文有介绍。。

---

### 并发问题
#### 现象和原因
这类问题现象比较复杂，[笔者](http://weibo.com/geecoodeer)尝试进行总结下。常见的现象是

* 业务系统无响应或者响应缓慢，CPU使用率100%（在多核情况下，可以通过 top 1看到各核使用的情况，通常可以看到某核）。
* 单线程情况下，业务操作没问题，并发量高的时候就数据紊乱，会报校验出错，数据结果不对。
* 出现Resource unavailable，有时候无法ssh到服务器上。

常见原因如下：

* 出现了死循环
	* 无限递归
	* HashMap死循环
* 使用有状态服务，并发情况下数据紊乱
* 线程过多,没有限制个数。



#### 定位方法
在真正进行解决问题前，如果对多线程不清楚的话，可以看看笔者的[《JAVA学习笔记之多线程编程》](http://my.oschina.net/geecoodeer/blog/191776)系列文章。 

定位线程问题也有一个神器，就是[IBM Thread and Monitor Dump Analyzer for Java](https://www.ibm.com/developerworks/community/groups/service/html/communityview?communityUuid=2245aa39-fa5c-4475-b891-14c205f7333c)。一般线程数，死锁，锁争用情况能够一览无余。

本工具使用起来比较方便，但是需要对线程状态有个

#### 典型的业务案例 
由于多线程编程的确比较复杂，这个有必要举几个案例详细说一下。
	
把jca 重复方法栈合并，显示个数。

容错问题。

 
 
日志分析 应用日志，重启； 

没有设置socket time 

调查现状,明确问题的现象
问题解决:
系统架构
代码实现
业务功能,聊天记录,线下分析

内存过大
 	锁粒度控制不够

框架使用了缓存，没有重新新建对象实例。 	
 	
如何利用工具分析:
CPU使用率,高 线程状态 

常见API耗时(wenshao,截图,chenshuo)
日照的机房速度
r大截图 String size 大小 1.6，1.7中有变化。
对象大小 
PERFTOOL 
MAT 
JPROFILER
JVISUALVM 

1KB 近似 1000
网络速度大于磁盘随机读写速度。
分布式缓存


free -m ,

业务bug
classloader
drools api 误用，框架缺陷， 

jdbc 大量无效字节。 soLinger Javainfo

http://hellojava.info/

BTRACE，打印日志信息不全，e.toString

傻瓜式性能定位：

 	
数据库索引 
表结构设计不合理,没有适当荣誉
paypal架构  

free -m 的误区 ,top 1各核使用状态,shift+m 内存使用排序

cdn系统，dns解析，网络ping，整体还是部分失效？ 监控。

不确定的时候，是各种工具互相结合使用。各方面知识交互起来。

定位过程高效很重要，解决方法相对而言，却有多种方式。

cpu 高，runnable incoming、outcoming  

先问，os慢，系统慢，局部功能慢
### TPS上不去

性能问题
dapper，系统监控,单机系统直接jprofiler看看hotspot热点代码，方法是否调用了多次，是否没走索引，是否没有缓存。 架构上的调整，算法等等。 随机读，顺序读。 合并写，对操作系统要求。全局锁的使用。  

对操作系统的理解,更底层细粒度的性能调优需要看下vtune,算法和汇编.

寻找瓶颈点,三个方面验证, 内存,线程和业务日志要互相对应. 
连接超时和缓存

事务不断变化,发展的本质.看完effective java不知道看啥了.

jira宕机后,后台一直抱无法获得新connection错误,直接sqlplus xx/xx@xx 非常缓慢, 经过检查日志,发现后台抱xxx错误,经过google,发现listener日志文件过大, lsnrctl off , start ,然后就恢复了.

性能问题和故障定位

1.启动时死锁,生成多个线程,quartz查询数据   触发加载hibernate的buildSessionFactory,会使用hibernate的annatation的加载,导致hashMap死循环. 解决方法 static holder, 构造方法中加载, 自定义加载hbm.xml , 容器的listener加载顺序.

系统cpu,系统内存,disk io,网卡优化,socket io 目前没遇到过. 各核状态, 高可能是cas,cpu use,但是sys 高 ,可能是线程过多.  jvm 堆内存(各区溢出情况,mat使用技巧(1.history配置打开2.incoming/outcoming 3. shortest gc path 4.hashmap,string 扩容等),), jvm 线程 ,内存 dump 分析.  半夜定时触发gc , jvisualvm btrace（判断一些异常）

由于系统在启动时，会访问数据库。而由于自己封装了持久层代码，支持扫描指定目录下的配置文件。
原来系统启动时是放到serverlet中，结果其他dao加载时，此时hibernate还没有完成自定义目录文件的加载。导致系统查询不到数据。

### 一般业务bug
针对新手来讲，watchdog，firebug，前台请求，后台请求。常见的坑就是get请求被缓存了，debug不到断点。代码不同步。

需要保证系统首先加载  

对象大小

cpu 100% 百分百的问题 ，由于hibernate的注解HashMap ，

某个历史遗留系统，每个数据是个短事务。突然有人加了长事务代码，导致系统的一个多线程的threadlocal获取数据数据为空，查询不到数据。

对象大小,对象头. infoq的几篇文章

参考
http://www.eecs.berkeley.edu/~rcs/research/interactive_latency.html
https://gist.github.com/jboner/2841832

new Object()只需要10条指令了,速度已经超过C(还是C++)了?


## 明确故障现象
常常接到现场的反馈就是系统慢，这里要善于和现场沟通，是哪里慢？整个系统慢？

## 找到故障根因
有时候无法直接找到，
储备好必要的理论，找到合适的工具。

## 实施修改
灰度



## 基础背景知识
### 常见时延大小


````
Latency Comparison Numbers
--------------------------
L1 cache reference                            0.5 ns
Branch mispredict                             5   ns
L2 cache reference                            7   ns             14x L1 cache
Mutex lock/unlock                            25   ns
Main memory reference                       100   ns             20x L2 cache, 200x L1 cache
Compress 1K bytes with Zippy              3,000   ns
Send 1K bytes over 1 Gbps network        10,000   ns    0.01 ms
Read 4K randomly from SSD*              150,000   ns    0.15 ms
Read 1 MB sequentially from memory      250,000   ns    0.25 ms
Round trip within same datacenter       500,000   ns    0.5  ms
Read 1 MB sequentially from SSD*      1,000,000   ns    1    ms  4X memory
Disk seek                            10,000,000   ns   10    ms  20x datacenter roundtrip
Read 1 MB sequentially from disk     20,000,000   ns   20    ms  80x memory, 20X SSD
Send packet CA->Netherlands->CA     150,000,000   ns  150    ms
 
````

### 各种API性能以及性能优化技巧

[点击下载 各种API性能_性能优化技巧](http://vdisk.weibo.com/s/rPtPB/1361865333/ ) 

{% include JB/setup %}
