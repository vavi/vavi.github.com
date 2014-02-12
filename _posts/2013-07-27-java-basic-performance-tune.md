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
3. 查看系统运行情况，比如应用日志，jvm内存，线程，操作系统的等情况。
	1. 操作系统情况(CPU，内存，磁盘，网络)
	2. JVM内存，线程，GC日志
	3. 业务系统日志	
4. 针对性分析。 

---

## 案例分享
### 内存泄露
#### 现象和原因
这类故障的典型现象是系统无响应，功能基本全部瘫痪。常见原因包括：

* 查询数据库结果集过大，没有进行分页处理。
* 过大的缓存对象占用了大量的内存空间
* 或者是频繁创建了大对象，导致JVM新生代和老年代处于100%，系统频繁FullGC。
* 堆外内存泄露，可以参考[perftools查看堆外内存泄露](http://koven2049.iteye.com/blog/1142768)

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
在真正进行解决问题前，如果对多线程不清楚的话，可以先看看笔者的[《JAVA学习笔记之多线程编程》](http://my.oschina.net/geecoodeer/blog/191776)系列文章。 

定位线程问题也有一个神器，就是[IBM Thread and Monitor Dump Analyzer for Java](https://www.ibm.com/developerworks/community/groups/service/html/communityview?communityUuid=2245aa39-fa5c-4475-b891-14c205f7333c)。一般线程数，死锁，锁争用情况能够一览无余。

本工具使用起来比较方便，但是需要对线程状态有个基本认识：

* RUNNABLE，处于执行状态。
* BLOCKED，处于受阻塞并等待某个监视器锁状态。
* WAITING，处于无限期地等待另一个线程来执行某一特定操作状态。

RUNNABLE处于占有锁状态，BLOCKED处于等待获取锁状态，WAITING处于被别人通知状态。也就是说，如果某个core使用率处于CPU100%的话，通常只需要关注RUNNABLE状态。

#### 典型的业务案例 
由于多线程编程的确比较复杂，这个有必要举几个案例，大概说一下。
	
* HashMap死循环。某业务代码，一个类内部使用HashMap来封装动态变量。刚开始，是单线程执行。后来，为了提升运行性能，把某段逻辑改成了异步。然后，有一天终于被坑了。
* HashMap死循环。某业务代码，自行封装hibernate框架。然后在系统启动时，时不时发现系统启动假死。通过通过查看CPU使用率以及线程堆栈，发现存在多个线程同时加载hibernate，然后触发hibernate的annatation组件加载。而这个annatation组件内部使用了HashMap，从而发生hashMap死循环。解决方法通过引入static holder单例模式，确保只有一个线程在初始化hibernate框架。
* 有状态服务。某RPC通信框架，直接反射反射创建实例。后来据说为了提升反射性能，把反射后的实例该缓存了，这个相当于这个类变成单例了，但是业务方并没有遵守“无状态服务”这个约定。然后，有一天终于被坑了。
* 线程过多。某业务逻辑，直接for循环创建大量线程，jstack后发现创建了好几k个线程。

---

### TPS上不去
#### 现象和原因
一般在业务系统压力测试时，随着压力的上升，系统的吞吐量逐渐增加。直到到达拐点后，系统的吞吐量不升反降。

从原因上分析，实在太多。有宏观上的，比如架构问题；有细节上的，比如算法实现问题。笔者知道这个话题太大了，只能根据自己的经验来谈几句。
#### 定位方法
从以己及彼这个角度来看，大概可以从如下3个方面入手：

* JVM代码执行效率：
	* JVM启动参数，垃圾回收算法（响应优先还是吞吐量优先） 
	* 结合jProfiler，VisualVM工具，看看hotspot代码，方法是否重复调用了多次，方法执行时间过长，避免频繁创建不必要的对象，充分利用操作系统的预读后写机制，尽量做到顺序写和顺序读，充分使用零拷贝机制，注意到伪共享问题，减少不必要的系统调用。
	* 结合MAT工具，查看是否存在大对象
	* 结合JCA工具，分析线程情况，是否存在大量线程等待锁情况，控制锁的粒度
* 计算机体系结构：
	* 硬件：CPU频率、几core、缓存大小，内存大小，磁盘、SSD读写速度
	* 操作系统：版本， 典型参数设置如句柄数，内存页大小，注意cpu us、sys百分比
	* 网络：结合rtt和bandwidth把网卡跑满，详细的可以参考[这个](http://hellojava.info/?p=109)
* 系统架构
	* SOA框架：提供分布式计算能力，把原来需要自身计算的部分转移到其他机器上
	* MQ：把不必要同步的计算异步化，队列化。
	* 缓存：是否没有缓存或者缓存策略不合理，对频繁变化的数据进行缓存
	* 存储：是否没走索引，或者索引设置不够合理；表结构设计不合理,没有适当冗余

 
性能优化涉及的知识太博大精深了，笔者仅仅是把通常需要考虑的因素列在这里。在JAVA性能优化方面，有2本经典的书供参考，就是《Effective JAVA》和《JAVA Performance》。

在优化的过程中，首先还是需要找到关键路径，迅速解决问题，这个对公司很重要。另一方面，从个人发展角度来看，需要多做些micro benchmark积累经验值。 

另外，性能优化很是对一个人综合素质的考验，需要很多方面的知识。比如对操作系统，CPU，各种算法数据结构。这方面笔者也在持续地学习中...
 
---

### 非主流bug
这个世界上没有完全正确的程序，因为其无法被证明。一部分bug是因为缺乏必要的校验，还有一部分是缺乏对周边环境的了解。简单列几个让自己印象深刻的把。 
  
* 对环境不熟
	* 把文件写在jboss的class目录下面，导致jboss自动加载，进而导致系统无响应。经过一番定位无功而返。最后仔细看了下应用日志，发现jboss在自动重启。最后把自动重启参数调整了从而解决此问题。
* 对jvm不熟
	* 某业务系统，需要支持动态webservice加载。后来发现每次修改后，都无法生效。后来还是通过自定义classloader解决。
* 对api不熟
	* drools api误用。里面的contains不是针对字符串，而是针对集合类。这个bug定位了很多天，刚开始以为是并发bug，因为前10次可以正常调用，但是后面的调用统统失败。后来，发现就这个contains有问题。再后来，把doc翻翻，用错了。
	* 网络编程时，没有设置合理的connection timeout 和 soTimeout值，当对方系统或者网络连接不可用时，导致占用大量线程。另外， 注意到Socket.setSoLinger(boolean on, int linger)这个linger参数单位是秒，在[API单位误解造成的严重故障](http://hellojava.info/?p=248)中有详细介绍。

---

## 附录
### JAVA对象大小
这个在R大的[Java 程序的编译，加载 和 执行](http://www.valleytalk.org/2011/07/28/java-%E7%A8%8B%E5%BA%8F%E7%9A%84%E7%BC%96%E8%AF%91%EF%BC%8C%E5%8A%A0%E8%BD%BD-%E5%92%8C-%E6%89%A7%E8%A1%8C/)里的P.124有详细介绍。

另外，多说一句，Oracle在Java 1.7.0_06版本里，更改String类中的字符串内部表示。详情点击[这里](http://www.infoq.com/cn/news/2013/12/Oracle-Tunes-Java-String) 
 
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
   
第一次看到这个数据时，网络速度居然大于磁盘随机读写速度，这个着实让我印象深刻。

### 各种API性能以及性能优化技巧

[文档 各种API性能_性能优化技巧](http://vdisk.weibo.com/s/rPtPB/1361865333 ) 

[视频 各种API性能_性能优化技巧](http://video.sina.com.cn/v/b/97945407-1014033883.html)

## 参考
 
[Latency Numbers Every Programmer Should Know](https://gist.github.com/jboner/2841832)

[Java程序员也应该知道的一些网络知识](http://hellojava.info/?p=109)


本文完，欢迎指正，讨论和分享 :)

{% include JB/setup %}
