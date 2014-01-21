---

layout: post
title: "Netty5源码分析--4.总结"
description: ""
category: 开源项目
tags: [netty]

---

## JAVA NIO 复习
请先参考我之前的博文[JAVA学习笔记--3.Network IO](http://my.oschina.net/geecoodeer/blog/191774)的 NIO（NonBlocking IO） SOCKET 章节。这里主要讲下JAVA NIO其中几个比较被忽略的细节，不求全，欢迎补充。

### API
Select 

当调用`ServerSocketChannel.accept();`时，如果该channel处于非阻塞状态而且没有等待(pending)的连接，那么该方法会返回null；否则该方法会阻塞直到连接可用或者发生I/O错误。此时实际上Client发送了connect请求并且服务端是处于non-blocking模式下，那么这个accept()会返回一个不为null的channel。

当调用`SocketChannel.connect()`时，如果该channel出于non-blocking模式，只有在本地连接下才可能立即完成连接，并返回true；在其他情况下，该方法返回false，并且必须在后面调用 `SocketChannel.finishConnect`方法。如果`SocketChannel.finishConnect`的执行结果为true，才意味着连接真正建立。

`SocketChannel.write()`方法内部不涉及缓冲，它直接把数据写到socket的send buffer中。

### 坑 

每次迭代selector.keys()完时，记得remove该SelectionKey，防止发生CPU100%的问题。

通常不该register OP_WRITE，一般来说socket 缓冲区总是可写的，仅在write()方法返回0时或者未完全写完数据才需要register OP_WRITE操作。当数据写完的时候，需要deregister  OP_WRITE。 socket空闲时，即为可写.有数据来时，可读。对于nio的写事件，只在发送数据时，如果因为通道的阻塞，暂时不能全部发送，才注册写事件`key.interestOps(key.interestOps() | SelectionKey.OP_WRITE);`。等通道可写时，再写入。同时判断是否写完，如果写完，就取消写事件即可`key.interestOps(key.interestOps() & ~SelectionKey.OP_WRITE);`。
空闲状态下，所有的通道都是可写的，如果你给每个通道注册了写事件，那么肯定是死循环了,导致发生CPU100%的问题。详见Netty的NioSocketChannel的父类方法setOpWrite()和clearOpWrite()方法。这个在iteye里面的帖子[Java nio网络编程的一点心得](http://marlonyao.iteye.com/blog/1005690)也有说明。

在register OP_CONNECT后，并且触发OP_CONNECT后，需要再deregister OP_CONNECT。详见Netty的NioEventLoop.processSelectedKey()方法后半部分。
  
另外，需要注意到Selector.wakeup()是一个昂贵的操作，一般需要减少是用。详见NioEventLoop.run()方法内的处理方式。 

还有各种各样的坑，实在太多了。详见[Java NIO框架的实现技巧和陷阱](http://www.blogjava.net/killme2008/archive/2010/11/22/338420.html)


## NIO操作在Netty里面的体现
虽然[笔者](http://weibo.com/geecoodeer)阅读完几个重要的交互过程，但是代码细节较多，无法体现NETTY对NIO的使用。于是画了两幅图，与读者共享。

事先说明下，下图图片上的标记2表示是一个新的EventLoop线程，非main线程。

另外，虽然Netty中大量使用了ChannelFuture异步，但是换种方式理解的话，可以理解为同步执行。

### 服务端启动
![]({{ BASE_PATH }}/images/netty/Netty Server Start.png )
### 客户端启动
![]({{ BASE_PATH }}/images/netty/Netty Client Start.png )
### 服务端处理请求
这里有个细节，就是boss线程注册了OP_ACCEPT线程，然后当接收到客户端的请求时，会使用work线程池里面的线程来处理客户端请求。代码细节在`NioServerSocketChannel.doReadMessages`的tag1.1.2处以及`child.unsafe().register(child.newPromise());`处。
   
      
## Netty整体架构
### 模块结构
从Netty的[Maven仓库](http://mvnrepository.com/artifact/io.netty/netty-all/5.0.0.Alpha1)可以看
到，大致分为如下模块：

* netty-transport：框架核心类Channel,ChannelPipeline,ChannelHandler都是在这个模块里面定义的，提供了对不同协议支持框架。
* netty-buffer：对ByteBuffer的抽象，可以理解为网络中的数据。
* netty-codec：对不同格式的数据进行编码，解码。
* netty-handler：对数据进行处理的handler
* netty-common：公共处理类。

综上，可以看出，首先对框架机制和传输的数据进行了抽象，然后又对数据如何在框架中传输进行了抽象。

## 他山之石
下面的介绍的思想比较零散，不成系统，主要是阅读代码过程中产生的碰撞，供读者参考。

### 好的设计

Netty 同样也采用了多个Selector并存的结构，主要原因是单个Selector的结构在支撑大量连接事件的管理和触发已经遇到瓶颈。	

Bootstrap.channel()方法通过传递class对象，然后通过反射来实例化具体的Channel实例,一定程度上避免了写死类名字符串导致未来版本变动时发生错误的可能性。

框架必备的类，大家还可以看下common工程，里面真是一个宝藏。

* `InternalLogger`用来避免对第三方日志框架的依赖，如slf，log4j等等。
* `ChannelOption`灵活地使用泛型机制，避免用户设置参数发生低级错误。
* `SystemPropertyUtil`提供对xt属性的访问方法
* `DefaultThreadFactory`提供自定义线程工程类，方便定位问题。
* `PlatformDependent`如果需要支持不同平台的话，可以把平台相关的操作都放在一起进行管理。

打印日志时，可以参考这样来拼接参数，`String.format("nThreads: %d (expected: > 0)", nThreads)`

@Sharable表示该类是无状态的，仅仅起“文档”标记作用。

在子接口里仅仅把父接口方法返回值覆写了，然后什么都不做。这样一定程度上避免了强制转型的尴尬。

把前一个future作为下一个调用方法的参数，这样可以异步执行。然后后面的逻辑可以先判断future结果后再进行处理，从而提升性能。
   
### 中立
addLast0 等以0为结尾的方法表示私有的含义。

有些方法的getter、setter前缀省略，有点类似jQuery里面命名风格。不过不太建议在自己的产品中使用这种风格，尽量使用和和产品内的一样的命名风格。

### 疑惑

心中存疑，请大家不吝赐教。

在NioEventLoop.run()方法中，好像每次用完SelectionKey没有remove 掉它，可能和SelectedSelectionKeySet实现机制有关。但是没直接看出来具体之间的联系，

为什么boss也要是个线程池？目前来看，服务端2个线程保持不变，main线程出于wait状态，boss线程池其中的一个线程进行接收客户端连接请求，然后把请求转发给worker线程池。

`javaChannel().register(eventLoop().selector, 0, this);`,为什么ops参数默认是0？

`socketChannel.register() and key.interestOps()`还是有点不太明白，估计我的思路钻进死胡同了。

### 瑕疵

b.bind(port)这个里面的内容非常复杂，不仅仅是bind一个port那么简单。所以该方法命名不是很好。

父接口依赖子接口，也不是很好。

DefaultChannelPipeline.addLast 这个方法太坑了，并不是把handler加到最后一个上面，而是加到tail前一个。

无一类外的是，继承体系相对复杂。父类，子类的命名通常不能体现出谁是父类，谁是子类，除了一个Abstract能够直接看出来。

###  注意事项
客户端单实例，防止消耗过多线程。这个在[http://hellojava.info/](http://hellojava.info/)中多次提到。
 
## 其他

目前网上的NIO例子都基本都不太靠谱，BUG多多。建议可以参考下《JAVA 分布式JAVA应用 基础与实践》。

JAVA NIO不一定就比OIO快，重点是更加Scalable。

框架帮趟坑，不要轻易制造轮子，除非现有轮子很差劲。JAVA NIO里面的坑太多了，Netty里面的大量的issue充分说明了问题。技术某种程度上不是最重要的，如果大家努力程度差不多的话，技术上不会差到哪里去。开源产品主要是生态圈的建设。
 
接收对端数据时，数据通过netty从网络中读取，进行其他各种处理，然后供应用程序使用。发送本地数据时，应用程序首先完成数据处理， 然后通过netty进行各种处理，最终把数据发出。但是为什么要区分inbound,outbound，或者说提供了head->tail以及tail->head的遍历？  

当我们跳出里面的细节时，考虑一下，如果你是作者的话，会如何考虑。整体的一个算法 。不同的通讯模型，nio，sun jdk bug, option(默认和用户设置)，异步future、executor，pipeline、context、handler， 设计模式 。

我想象中操作应该是这样的，handler链管理不需要走pipeline，event链也不需要走pipiline，仅仅对数据的发送，接收和操作才走pipeline。事件机制应该起一些增强型、辅助型作用，不应该影响到核心流程的执行或者起到什么关键作用，主要是应该给第三方扩展用。而netty中，所有的一切都要走pipelne。

太多异步，怎么测试？

招式vs心法。招式，相当于api；心法，相当于api工作原理，利与弊。还是要理解底层，否则还是可能理解不清楚。cpu，内存，io，网络，而最终浮于招式。

看完了么？知道了Netty是什么，内部大概是怎么运作的，但是有些细节还不知道为什么。真的就理解精髓了么？NIO的坑.. 底层OS的坑.. Netty的精髓是在于对各种细节的处理，坑的处理，对性能的处理，而不是仅仅一个XX模式运用。JAVA NIO细节和坑实在太多，估计再给我一周的时间，也研究不完。有点小沮丧。

## 参考
[Blogjava：Java NIO框架的实现技巧和陷阱](http://www.blogjava.net/killme2008/archive/2010/11/22/338420.html)

[Stackoverflow：Java NIO - using Selectors](http://stackoverflow.com/questions/20966142/java-nio-using-selectors/20967805#20967805)

[Stackoverflow：Difference between socketChannel.register() and key.interestOps()](http://stackoverflow.com/questions/20972436/difference-between-socketchannel-register-and-key-interestops)

[WIKI：Event loop](http://en.wikipedia.org/wiki/Event_loop)

{% include JB/setup %}
