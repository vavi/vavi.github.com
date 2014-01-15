---
layout: post
title: "Netty5源码分析--0.核心领域概念"
description: ""
category: 开源项目
tags: [netty]
---

## Netty是什么
由于通用的协议或者实现有时不能满足各种各样的需求，比如我们通常不会用一个HTTP Server来同时进行传输大文件，email以及近实时的消息如金融信息和多玩家游戏数据。我们需要一个高度优化的协议实现，来满足一些特定的需求。

Netty是一个**异步，事件驱动**的网络应用**框架**，并且提供了一些工具来帮助迅速地开发**高性能，高扩展性**的服务端和客户端。

---
  
## 核心领域概念介绍
### 隐喻
Channel 公路，双向道，分为去路和来路。隐喻为网络连接。  

ChannelPipeline，公路管理部门，可以设置收费站，关卡等设施对车辆进行检查。

ByteBuf 车辆。隐喻为网络连接中的数据。
 
Handler 收费站，关卡；可以对公路上的车辆进行各种处理。

 
---
 
### Channel
顶层接口，因为有不同的通讯协议，比如TCP、UDP、rxtx等等以及不同的通讯模型，如OIO,NIO,AIO；所以针对这些不同的实现来提供一个统一的接口，在必要的时候，进行向下转型（Downcast），方便扩展。

该接口提供如下功能： 
当前channel的状态（比如是否open，connected）
当前channel的配置参数（比如receive buffer size）
当前channel支持的操作（比如read, write, connect, and bind）

另外，仍然需要值的一提是，Netty中的所有I/O操作都是异步的。这意味着这些I/O方法被调用时，会立即返回`ChannelFuture`，并不保证这些I/O操作实际完成（比如将数据已经发送到对端）。`ChannelFuture`会告诉你这个I/O操作结果是成功，失败还是取消。
 
`Channel`有一个parent。比如，`SocketChannel`是`ServerSocketChannel`的"accept方法"的返回值（ServerSocketChannel中并没有accept方法，但是在ServerSocket中是存在的）。所以，`SocketChannel`的 parent()返回`ServerSocketChannel`。

### ChannelPipeline
`ChannelPipeline`，顾名思义，意为“Channel的流水线”，处理了`Channel`中所有的I/O事件和请求。`ChannelPipeline`该类通过了实现[Intercepting Filter pattern](http://www.oracle.com/technetwork/java/interceptingfilter-142169.html)，结合`ChannelHandler`接口的实现类，提供了类似将流水线的工作分成多个步骤的功能。每个`ChannelHandler`接口的实现类完成了一定的功能，并且可以灵活地在流水线上增加，替换，删除Handler，极大地提高了框架的灵活性。

每个`Channel`在实例化时，会自动创建一个`ChannelPipeline`实例。

Inbound 通常用来表示从外界读入数据，OutBound通常用来表示将数据写出到外界。在TCP/IP协议栈中，数据需要从操作系统的栈读入，数据写出时也需要经过操作系统的TCP/IP栈。在Netty的实现中，只有一个Handler链，将时间触发，数据发送和数据接收结合在一起。但这个也是有点费解。

在触发Inbound I/O Event时，handler依照从0到N-1顺序被netty框架触发；在触发Outbound I/O Event时，handler依照从N-1到0顺序被netty框架触发。

Inbound event传播的方法 :
 
* `ChannelHandlerContext.fireChannelRegistered()`
* `ChannelHandlerContext.fireChannelActive()`
* `ChannelHandlerContext.fireChannelRead(Object)`
* `ChannelHandlerContext.fireChannelReadComplete()`
* `ChannelHandlerContext.fireExceptionCaught(Throwable)`
* `ChannelHandlerContext.fireUserEventTriggered(Object)`
* `ChannelHandlerContext.fireChannelWritabilityChanged()`
* `ChannelHandlerContext.fireChannelInactive()`

Outbound event 传播的方法 :

* `ChannelHandlerContext.bind(SocketAddress, ChannelPromise)`
* `ChannelHandlerContext.connect(SocketAddress, SocketAddress, ChannelPromise)`
* `ChannelHandlerContext.write(Object, ChannelPromise)`
* `ChannelHandlerContext.flush()`
* `ChannelHandlerContext.read()`
* `ChannelHandlerContext.disconnect(ChannelPromise)`
* `ChannelHandlerContext.close(ChannelPromise)`

read和write方法都放在Outbound event，这个真让我丈二和尚摸不着头脑哈。
  
`ChannelPipeline`是线程安全的，所以`ChannelHandler`可以在任意时刻被增加，移除和替换。

### EventLoopGroup 和 EventLoop
类继承关系如下，`JUC.ScheduledExecutorService`<--`EventExecutorGroup`<--`EventLoopGroup`<--`EventLoop`。这个"<--"表示是符号的右边继承符号的左边接口，下同。

`JUC.ScheduledExecutorService`是JUC框架的线程调度框架。

`EventExecutorGroup`增加了`next（）`和`children()`方法；另外还Deprecated了父接口的`shutdown()`和`shutdownNow()`方法，新增了`isShuttingDown()`，`shutdownGracefully()`，`shutdownGracefully()`，`terminationFuture()`。

`EventLoopGroup` 覆写了`EventExecutorGroup`的`EventLoop next()`方法。

`EventLoop`在继承了`EventLoopGroup`的同时，也继承了`EventExecutor`接口。所以这里补充介绍下`EventExecutor`类，它继承了`EventExecutorGroup`,另外，它主要新增了`EventExecutorGroup parent()`方法和`boolean inEventLoop(Thread thread)`方法。

`EventLoop`自身主要新增了 `ChannelHandlerInvoker asInvoker()`方法，它主要负责处理所有I/O操作。

从职责上讲，`EventLoop`表示事件循环的意思，也就是死循环来捕获不同的事件，比如说是否可读，可写等等。`EventExecutorGroup`则表示一组`EventLoop`，所以它里面才有`next（）`和`children()`方法。另外，这几个类有点乱，尤其是父接口`EventExecutorGroup`依赖了子接口`EventLoop`，违反了DIP原则。

### Future，ChannelFuture 和 ChannelPromise

`ChannelFuture`表示实际的I/O操作还未发生。因为在netty中，所有的I/O操作（如read, write, connect,和bind）都是异步的。类继承关系如下，`JUC.Future`<--`Future`<--`ChannelFuture`<--`ChannelPromise`

`Future`继承了JUC框架里的Future接口，还主要新增了`isSuccess()`,`isCancellable()`,`cause()`方法；此外还新增了注册Future完成的时候触发对应的Listener的功能

`ChannelFuture`主要新增了`Channel channel()`方法。

`ChannelPromise`在继承`ChannelFuture`的同时，还继承了`Promise`接口。该`Promise`主要增加了对`Future`的写方法，如 `Promise<V> setSuccess(V result)`等。也就是说，`ChannelPromise`基本是`ChannelFuture`和`Promise`的结合体。

也就是说，`ChannelFuture`不仅细化了`JUC.Future`语义，可以方便知道到底是成功，取消还是异常，还提供了`Channel channel()`。`ChannelPromise`则可以根据程序的运行结果设置一些业务含义。另外，也可以观察到，在子接口里仅仅把父接口方法返回值覆写了，然后什么都不做。这样一定程度上避免了强制转型的尴尬。

### ByteBuf
`ByteBuf`支持随机和顺序访问内部的字节。它具有readerIndex和writerIndex。它们的关系如下：

      +-------------------+------------------+------------------+
      | discardable bytes |  readable bytes  |  writable bytes  |
      |                   |     (CONTENT)    |                  |
      +-------------------+------------------+------------------+
      |                   |                  |                  |
      0      <=      readerIndex   <=   writerIndex    <=    capacity
 

建议使用 Unpooled等helper方法来创建`ByteBuf`对象，而不是直接使用`ByteBuf`的实现的构造器。在5.0版本，建议使用`PooledByteBufAllocator`来创建`ByteBuf`对象。

### Bootstrap 和 ServerBootstrap
Bootstrap这个词在计算机中，通常表示某个框架开始执行的第一段代码。详见[What is bootstrapping?](http://stackoverflow.com/questions/1254542/what-is-bootstrapping)

在Netty中，Bootstrap是客户端启动类，ServerBootstrap是服务端启动类，用来帮助程序员迅速开始工作。
 
--- 
## 参考
[User Guide For 5.x](http://netty.io/wiki/user-guide-for-5.x.html)


{% include JB/setup %}