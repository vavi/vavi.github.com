---
layout: post
title: "netty5源码分析"
description: ""
category: 开源项目
tags: [netty]
---

## 用户手册重点介绍
通用的协议或者实现有时不能满足各种各样的需求。就好像，我们通常不会使用一个HTTP Server来同时进行传输大文件，email以及近实时的消息如金融信息和多玩家游戏数据。我们需要一个高度优化的协议实现，来满足一些特定的需求。

Netty是一个**异步，事件驱动**的网络应用框架，并且提供了一些工具来帮助迅速地开发**高性能，高扩展性**的服务端和客户端。

下面的介绍不成系统，有点离散，请各位看官先有个初步概念，如果有问题，可以先翻下UserGuide。

`ChannelHandlerAdapter` 实现了`ChannelHandler`接口 ，`ChannelInitializer`继承了`ChannelHandlerAdapter`。`ChannelHandlerAdapter`大部分职责委托给`ChannelHandlerContext`实现类

`ByteBuf`是一个引用计数对象，必须显式地调用`release()`方法来减少引用计数。  
   
Boss线程池用来接受客户端的连接请求，Worker线程池用来处理boss线程池里面的连接的数据。
   
`ChannelInitializer`是一个特殊的handler，用来初始化ChannelPipeline里面的handler链。 这个特殊的`ChannelInitializer`在加入到pipeline后，在initChannel调用结束后会被remove掉，从而完成初始化的效果（后文会详述）。

`AbstractBootstrap.option()`用来设置ServerSocket的参数，`AbstractBootstrap.childOption()`用来设置Socket的参数。
 
在调用ctx.write(Object)后需要调用ctx.flush()方法，这样才能将数据发出去。或者直接调用 ctx.writeAndFlush(msg)方法。
 
`final ByteBuf time = ctx.alloc().buffer(4); ` 

`ChannelFuture`表示实际的I/O操作还未发生。因为在netty中，所有的I/O操作（如read, write, connect,和bind）都是异步的。 
 
在处理基于流的传输协议TCP/IP的数据时，接收的数据是存储在socket接受缓冲里面的。不幸的是，假如你发送了2个报文，底层是发送了两组字节，这意味着，操作系统仅仅把这两个报文当成一组字节来处理，它并未区分那一部分字节是第一个报文，哪一部分是第二个字节。这些数据需要业务程序自己处理，比如当接收的数据满足4个字节时，业务程序才进行处理，否则就一直等待。

---
  
## 核心领域概念介绍
### 架构
TODO 
### 隐喻
Channel 公路，双向道，分为去路和来路。隐喻为网络连接。 

ByteBuf 车辆。隐喻为网络连接中的数据。

ChannelPipeline，公路管理部门，可以设置收费站，关卡等设施对车辆进行检查。

Handler 收费站，关卡；可以对公路上的车辆进行各种处理。

---

### 核心类说明
#### Channel
顶层接口，因为有不同的通讯协议，比如TCP、UDP、rxtx等等以及不同的通讯模型，如OIO,NIO,AIO；所以针对这些不同的实现来提供一个统一的接口，在必要的时候，进行向下转型（Downcast），方便扩展。

该接口提供如下功能： 
当前channel的状态（比如是否open，connected）
当前channel的配置参数（比如receive buffer size）
当前channel支持的操作（比如read, write, connect, and bind）

另外，仍然需要值的一提是，Netty中的所有I/O操作都是异步的。这意味着这些I/O方法被调用时，会立即返回`ChannelFuture`，并不保证这些I/O操作实际完成（比如将数据已经发送到对端）。`ChannelFuture`会告诉你这个I/O操作结果是成功，失败还是取消。
 
`Channel`有一个父亲。比如，`SocketChannel`是`ServerSocketChannel`的"accept方法"的返回值（ServerSocketChannel中并没有accept方法，但是在ServerSocket中是存在的）。所以，`SocketChannel`的 parent()返回`ServerSocketChannel`。

#### ChannelPipeline
`ChannelPipeline`，顾名思义，意为“Channel的流水线”，处理了`Channel`中所有的I/O事件和请求。`ChannelPipeline`该类通过了实现[Intercepting Filter pattern](http://www.oracle.com/technetwork/java/interceptingfilter-142169.html)，结合`ChannelHandler`接口的实现类，提供了类似将流水线的工作分成多个步骤的功能。每个`ChannelHandler`接口的实现类完成了一定的功能，并且可以灵活地在流水线上增加，替换，删除Handler，极大地提高了框架的灵活性。

每个`Channel`在实例化时，会自动创建一个`ChannelPipeline`实例。

Inbound 通常用来表示从外界读入数据，OutBound通常用来表示将数据写出到外界。在TCP/IP协议栈中，数据需要从操作系统的栈读入，数据写出时也需要经过操作系统的TCP/IP栈。在Netty的实现中，只有一个Handler链，将数据发送和数据接收结合在一起。但这个也是有点费解。

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

#### EventLoopGroup 和 EventLoop
`ScheduledExecutorService`<--`EventExecutorGroup`<--`EventLoopGroup`<--`EventLoop`
这个"<--"表示是符号的右边继承符号的左边接口。

`ScheduledExecutorService`是JUC框架的线程调度框架。

`EventExecutorGroup`增加了`next（）`和`children()`方法；另外还Deprecated了父接口的`shutdown()`和`shutdownNow()`方法，新增了`isShuttingDown()`，`shutdownGracefully()`，`shutdownGracefully()`，`terminationFuture()`。

`EventLoopGroup` 覆写了`EventExecutorGroup`的`EventLoop next()`方法。

`EventLoop`在继承了`EventLoopGroup`的同时，也继承了`EventExecutor`接口。所以这里补充介绍下`EventExecutor`类，它继承了`EventExecutorGroup`,另外，它主要新增了`EventExecutorGroup parent()`方法和`boolean inEventLoop(Thread thread)`方法。

`EventLoop`自身主要新增了 `ChannelHandlerInvoker asInvoker()`方法，它主要负责处理所有I/O操作。

小结下，这几个类有点乱，尤其是父接口`EventExecutorGroup`依赖了子接口`EventLoop`，并且感觉这几个类职责并不是很清晰，边界不是很清楚。

#### Future，ChannelFuture 和 ChannelPromise
`JUC.Future`<--`Future`<--`ChannelFuture`<--`ChannelPromise`

`Future`继承了JUC框架里的Future接口，还主要新增了`isSuccess()`,`isCancellable()`,`cause()`方法；此外还新增了注册Future完成的时候触发对应的Listener的功能

`ChannelFuture`主要新增了`Channel channel()`方法。

`ChannelPromise`在继承`ChannelFuture`的同时，还继承了`Promise`接口。该`Promise`主要增加了对`Future`的写方法，如 `Promise<V> setSuccess(V result)`等。也就是说，`ChannelPromise`基本是`ChannelFuture`和`Promise`的结合体
 
另外，可以观察到，在子接口里仅仅把父接口方法返回值覆写了，然后什么都不做。这样一定程度上避免了强制转型的尴尬。

#### ByteBuf
`ByteBuf`支持随机和顺序访问内部的字节。它具有readerIndex和writerIndex。它们的关系如下：

      +-------------------+------------------+------------------+
      | discardable bytes |  readable bytes  |  writable bytes  |
      |                   |     (CONTENT)    |                  |
      +-------------------+------------------+------------------+
      |                   |                  |                  |
      0      <=      readerIndex   <=   writerIndex    <=    capacity
 

建议使用 Unpooled等helper方法来创建`ByteBuf`对象，而不是直接使用`ByteBuf`的实现的构造器。在5.0版本，建议使用`PooledByteBufAllocator`来创建`ByteBuf`对象。

#### Bootstrap 和 ServerBootstrap
Bootstrap这个词在计算机中，通常表示某个框架开始执行的第一段代码。详见[What is bootstrapping?](http://stackoverflow.com/questions/1254542/what-is-bootstrapping)

在Netty中，Bootstrap是客户端启动类，ServerBootstrap是服务端启动类，用来帮助程序员迅速开始工作。
 
--- 
 
## 服务端启动服务
### 背景介绍
先初步有个印象，后面讲到时会再次提及。先讲整体，看看悟性，不懂的话后面会再讲到细节代码时会再谈到。

1. `Channel`<--`AbstractChannel`<--`AbstractNioChannel`<--`AbstractNioMessageChannel`<--`AbstractNioMessageServerChannel`<--`NioServerSocketChannel	`，类继承关系如上，相对比较清晰。
2. `Unsafe`<--`NioUnsafe`<--`AbstractNioUnsafe`<--`NioMessageUnsafe`。在实现上，`AbstractUnsafe`是`AbstractChannel`的内部类。


可能还需要挪到上面一个章节。TODO

### NioEventLoopGroup初始化
1. `NioEventLoopGroup`初始化父类`MultithreadEventLoopGroup`,触发父类获得默认的线程数，其值默认是`Runtime.getRuntime().availableProcessors() * 2`
2. 然后接着调用`MultithreadEventExecutorGroup(int nThreads, Executor executor, Object... args)`构造方法。依次触发如下步骤：
	1. 初始化`private final Promise<?> terminationFuture = new DefaultPromise(GlobalEventExecutor.INSTANCE);`属性；里面水较深，后续分析TODO。
	2. 设置默认DefaultThreadFactory线程工厂，主要做了2件事，设置线程池名称和线程名称
	3. 初始化children数组，然后通过调用`NioEventLoopGroup.newChild`方法完成child属性设置。这个方法比较重要，先要介绍下：
		1. 对象继承关系如下，`JUC.AbstractExecutorService`<--`AbstractEventExecutor`<--`SingleThreadEventExecutor`<--`SingleThreadEventLoop`<--`NioEventLoop`
		2. 然后在类加载时，主要涉及到对象的初始化。其中一个是在`NioEventLoop`的静态块中解决了JDK 的一个bug
		3. 设置`NioEventLoop.parent`为`NioEventLoopGroup`
		4. 调用`NioEventLoop.openSelector()`，完成selector初始化
			1. 初始化`SelectedSelectionKeySet`，设置其属性` keysA = new SelectionKey[1024]; keysB = keysA.clone();`
			2. 进行了一个优化，设置了`sun.nio.ch.SelectorImpl`的`selectedKeys`和`publicSelectedKeys`属性。用意何在？TODO
   4. 循环完成children数组的初始化
3. `NioEventLoopGroup`初始化完毕。   


小结：此时结合Eclipse的DEBUG视图，观察bossGroup的属性，可以基本看到完成如下几个事情

* 设置默认线程数和默认线程工厂
* 设置`NioEventLoop`的selector属性

---

### ServerBootstrap 初始化

	ServerBootstrap b = new ServerBootstrap();
    b.group(bossGroup, workerGroup)
    .channel(NioServerSocketChannel.class)
    .childHandler(new TelnetServerInitializer());	
上面这段代码内涵平平，主要设置group属性是bossGroup，childGroup属性是workerGroup。
没啥其他复杂属性赋值。主要值得一提的就是channel方法的设计，通过传递class对象，然后通过反射来实例化具体的Channel实例。

`b.bind(port).sync().channel().closeFuture().sync();`

b.bind(port)方法会调用下面的doBind方法，在doBind方法中会完成Channel的初始化，包括实例化和参数设置

    private ChannelFuture doBind(final SocketAddress localAddress) {
        final ChannelFuture regFuture = initAndRegister();//重点介绍1
        final Channel channel = regFuture.channel();
        if (regFuture.cause() != null) {
            return regFuture;
        }

        final ChannelPromise promise;
        if (regFuture.isDone()) {
            promise = channel.newPromise();
            doBind0(regFuture, channel, localAddress, promise);//重点介绍2
        } else {
            // Registration future is almost always fulfilled already, but just in case it's not.
            promise = new DefaultChannelPromise(channel, GlobalEventExecutor.INSTANCE);
            regFuture.addListener(new ChannelFutureListener() {
                @Override
                public void operationComplete(ChannelFuture future) throws Exception {
                    doBind0(regFuture, channel, localAddress, promise);
                }
            });
        }

        return promise;
    }
 
 上文的 initAndRegister和doBind0需要细说。
   
    final ChannelFuture initAndRegister() {
        Channel channel;
        try {
            channel = createChannel();//重点介绍1.1
        } catch (Throwable t) {
            return VoidChannel.INSTANCE.newFailedFuture(t);
        }

        try {
            init(channel);//重点介绍1.2
        } catch (Throwable t) {
            channel.unsafe().closeForcibly();
            return channel.newFailedFuture(t);
        }

        ChannelPromise regFuture = channel.newPromise();
        channel.unsafe().register(regFuture);//重点介绍1.3
        if (regFuture.cause() != null) {
            if (channel.isRegistered()) {
                channel.close();
            } else {
                channel.unsafe().closeForcibly();
            }
        }
   
   重点介绍1.1,调用ServerBootstrap.createChannel() ，通过反射完成Channel初始化
   
    @Override
    Channel createChannel() {
        EventLoop eventLoop = group().next();
        return channelFactory().newChannel(eventLoop, childGroup);//重点介绍1.1.1

    }
    
重点介绍1.1.1，此时将断点打到NioServerSocketChannel的构造方法上     

	 public NioServerSocketChannel(EventLoop eventLoop, EventLoopGroup childGroup) {
        super(null, eventLoop, childGroup, newSocket(), SelectionKey.OP_ACCEPT);//重点介绍1.1.1.1
        config = new DefaultServerSocketChannelConfig(this, javaChannel().socket());//重点介绍1.1.1.2

    }

 重点介绍1.1.1.1,这段代码主要完成3件事。
 
 第一个是在`NioServerSocketChannel.newSocket()`调用了`ServerSocketChannel.open()`，完成了javaChannel的创建
   
        private static ServerSocketChannel newSocket() {
        try {
            return ServerSocketChannel.open();
        } catch (IOException e) {
            throw new ChannelException(
                    "Failed to open a server socket.", e);
        }
    }

第二个是在`AbstractNioChannel`的构造方法中调用了`ch.configureBlocking(false)`方法
 
	 protected AbstractNioChannel(Channel parent, EventLoop eventLoop, SelectableChannel ch, int readInterestOp) {
        super(parent, eventLoop);//重点介绍1.1.1.1.1
        this.ch = ch;
        this.readInterestOp = readInterestOp;
        try {
            ch.configureBlocking(false);
        } catch (IOException e) {
            try {
                ch.close();
            } catch (IOException e2) {
                if (logger.isWarnEnabled()) {
                    logger.warn(
                            "Failed to close a partially initialized socket.", e2);
                }
            }

            throw new ChannelException("Failed to enter non-blocking mode.", e);
        }
    }   
 
 重点介绍1.1.1.1.1中，在`AbstractChannel(Channel parent, EventLoop eventLoop)`中，进行了两个重要操作：` unsafe = newUnsafe();pipeline = new DefaultChannelPipeline(this);`。
 
 	protected AbstractChannel(Channel parent, EventLoop eventLoop) {
        this.parent = parent;
        this.eventLoop = validate(eventLoop);
        unsafe = newUnsafe();
        pipeline = new DefaultChannelPipeline(this);//重点介绍1.1.1.1.1.1
    }

重点介绍1.1.1.1.1.1,设置了HeadHandler和TailHandler。这两个类也比较重要。
    
   public DefaultChannelPipeline(AbstractChannel channel) {
        if (channel == null) {
            throw new NullPointerException("channel");
        }
        this.channel = channel;

        TailHandler tailHandler = new TailHandler();
        tail = new DefaultChannelHandlerContext(this, null, generateName(tailHandler), tailHandler);//重点介绍1.1.1.1.1.1.1

        HeadHandler headHandler = new HeadHandler(channel.unsafe());
        head = new DefaultChannelHandlerContext(this, null, generateName(headHandler), headHandler);

        head.next = tail;
        tail.prev = head;
    }  
 
 重点介绍1.1.1.1.1.1.1,这个方法完成了DefaultChannelHandlerContext的对象的初始化。这个类也是核心类，会在后面重点分析。
   
   
 此时，我们方法调用栈结束，然后回到 重点介绍1.1.1.2 这段代码上来。 在`DefaultServerSocketChannelConfig`中构造方法中完成了channel的参数设置
 
 至此，才完成重点介绍1.1 AbstractBootstrap.createChannel()方法的执行。现在又开始 重点介绍1.2的代码片段。该  AbstractBootstrap.init(Channel channel)  方法里面主要涉及到Parent Channel 和 Child Channel的option和attribute 设置，并将客户端设置的参数覆盖到默认参数中；最后，还将`childHandler(new TelnetServerInitializer())`中设置的handler加入到pipeline()中。代码见下。
 
void init(Channel channel) throws Exception {
        final Map<ChannelOption<?>, Object> options = options();
        synchronized (options) {
            channel.config().setOptions(options);
        }

        final Map<AttributeKey<?>, Object> attrs = attrs();
        synchronized (attrs) {
            for (Entry<AttributeKey<?>, Object> e: attrs.entrySet()) {
                @SuppressWarnings("unchecked")
                AttributeKey<Object> key = (AttributeKey<Object>) e.getKey();
                channel.attr(key).set(e.getValue());
            }
        }

        ChannelPipeline p = channel.pipeline();
        if (handler() != null) {
            p.addLast(handler());
        }

        final ChannelHandler currentChildHandler = childHandler;
        final Entry<ChannelOption<?>, Object>[] currentChildOptions;
        final Entry<AttributeKey<?>, Object>[] currentChildAttrs;
        synchronized (childOptions) {
            currentChildOptions = childOptions.entrySet().toArray(newOptionArray(childOptions.size()));
        }
        synchronized (childAttrs) {
            currentChildAttrs = childAttrs.entrySet().toArray(newAttrArray(childAttrs.size()));
        }

        p.addLast(new ChannelInitializer<Channel>() {
            @Override
            public void initChannel(Channel ch) throws Exception {
                ch.pipeline().addLast(new ServerBootstrapAcceptor(currentChildHandler, currentChildOptions,
                        currentChildAttrs));
            }
        });
    }


至此完成重点介绍1.2执行，开始执行重点介绍1.3 `channel.unsafe().register(regFuture);`这段代码。该方法内部接着执行执行重点介绍1.3.1的代码。

	public final void register(final ChannelPromise promise) {
            if (eventLoop.inEventLoop()) {
                register0(promise);
            } else {
                try {
                    eventLoop.execute(new Runnable() {
                        @Override
                        public void run() {//重点介绍1.3.1
                            register0(promise);
                        }
                    });
                } catch (Throwable t) {
                    logger.warn(
                            "Force-closing a channel whose registration task was not accepted by an event loop: {}",
                            AbstractChannel.this, t);
                    closeForcibly();
                    closeFuture.setClosed();
                    promise.setFailure(t);
                }
            }
        }

重点介绍1.3.1,该片段主要执行`doRegister();//重点介绍1.3.1.1`和` pipeline.fireChannelRegistered();//重点介绍1.3.1.2`


 	private void register0(ChannelPromise promise) {
            try {
                // check if the channel is still open as it could be closed in the mean time when the register
                // call was outside of the eventLoop
                if (!ensureOpen(promise)) {
                    return;
                }
                doRegister();//重点介绍1.3.1.1
                registered = true;
                promise.setSuccess();
                pipeline.fireChannelRegistered();//重点介绍1.3.1.2
                if (isActive()) {
                    pipeline.fireChannelActive();
                }
            } catch (Throwable t) {
                // Close the channel directly to avoid FD leak.
                closeForcibly();
                closeFuture.setClosed();
                if (!promise.tryFailure(t)) {
                    logger.warn(
                            "Tried to fail the registration promise, but it is complete already. " +
                                    "Swallowing the cause of the registration failure:", t);
                }
            }
        }

重点介绍1.3.1.1 将代码片段将javachannel注册到selector上，并把selectionKey属性赋值

 	protected void AbstractNioChannel.doRegister() throws Exception {
        boolean selected = false;
        for (;;) {
            try {
                selectionKey = javaChannel().register(eventLoop().selector, 0, this);
                return;
            } catch (CancelledKeyException e) {
                if (!selected) {
                    // Force the Selector to select now as the "canceled" SelectionKey may still be
                    // cached and not removed because no Select.select(..) operation was called yet.
                    eventLoop().selectNow();
                    selected = true;
                } else {
                    // We forced a select operation on the selector before but the SelectionKey is still cached
                    // for whatever reason. JDK bug ?
                    throw e;
                }
            }
        }
    }

重点介绍1.3.1.2，这个方法里面有一堆事情要讲。先暂且放下，后文再说。 
 
 public ChannelPipeline DefaultChannelPipeline.fireChannelRegistered() {
        head.fireChannelRegistered();
        return this;
    }
    
 此时终于完成 重点介绍1 代码片段执行，开始执行 重点介绍2 的代码片段。  
 
 private static void doBind0(
            final ChannelFuture regFuture, final Channel channel,
            final SocketAddress localAddress, final ChannelPromise promise) {

        // This method is invoked before channelRegistered() is triggered.  Give user handlers a chance to set up
        // the pipeline in its channelRegistered() implementation.
        channel.eventLoop().execute(new Runnable() {
            @Override
            public void run() {
                if (regFuture.isSuccess()) {
                    channel.bind(localAddress, promise).addListener(ChannelFutureListener.CLOSE_ON_FAILURE);重点介绍2.1
                } else {
                    promise.setFailure(regFuture.cause());
                }
            }
        });
    }
    
  重点介绍2.1，该方法内部有调用了pipeline的方法了。 好吧，是时候介绍pipeline了。 
  
   public ChannelFuture bind(SocketAddress localAddress, ChannelPromise promise) 	{
        return pipeline.bind(localAddress, promise);
    } 

#### Pipeline

	
	这个pipeline内部维护一个链表，依次是head,$1,tail。另外，内部还有name2ctx这个map属性，也就是说，这个类既提供了O(N)，也提供O(1)操作。
	
	
	
当我们跳出里面的细节时，考虑一下，你是作者的话，会如何考虑。整体的一个算法 。
 nio，sun jdk bug, option(默认和用户设置)，异步future、executor，pipeline、context、handler，nio，设计模式（模板），不同的通信，tcp，udp，
 
 
 
 
  
 
5. 在`AbstractBootstrap.doBind0()`中调用了`channel.bind(localAddress, promise).addListener(ChannelFutureListener.CLOSE_ON_FAILURE);`方法，完成了bind具体的服务器端口，最终调用了`NioServerSocketChannel.doBind(SocketAddress localAddress)`完成bind。接着触发`pipeline.fireChannelActive()`事件。

invokeChannelRegisteredNow 静态导入

 此时完成XX方法执行，现在回到XXX方法
	
 
 

    public ChannelHandlerContext fireChannelRegistered() {
        DefaultChannelHandlerContext next = findContextInbound(MASK_CHANNEL_REGISTERED);
        next.invoker.invokeChannelRegistered(next);
        return this;
    }
    
    public void invokeChannelRegistered(final ChannelHandlerContext ctx) {
        if (executor.inEventLoop()) {
            invokeChannelRegisteredNow(ctx);
        } else {
            executor.execute(new Runnable() {
                @Override
                public void run() {
                    invokeChannelRegisteredNow(ctx);
                }
            });
        }
    }     
    
  void io.netty.channel.ChannelInitializer.channelRegistered(ChannelHandlerContext ctx) throws Exception
  
  
  public final void channelRegistered(ChannelHandlerContext ctx) throws Exception {
        ChannelPipeline pipeline = ctx.pipeline();
        boolean success = false;
        try {
            initChannel((C) ctx.channel());
            pipeline.remove(this);
            ctx.fireChannelRegistered();
            success = true;
        } catch (Throwable t) {
            logger.warn("Failed to initialize a channel. Closing: " + ctx.channel(), t);
        } finally {
            if (pipeline.context(this) != null) {
                pipeline.remove(this);
            }
            if (!success) {
                ctx.close();
            }
        }
    }  
    
    
  然后调用initChannel方法，从而把ServerBootstrapAcceptor也加入到pipeline中了。
  
  	public void initChannel(Channel ch) throws Exception {
                ch.pipeline().addLast(new 			ServerBootstrapAcceptor(currentChildHandler, currentChildOptions,
                        currentChildAttrs));
            }
 
 这里的ServerBootstrapAcceptor同样继承于 ChannelHandlerAdapter，里面的属性childHandler就是 io.netty.example.telnet.TelnetServerInitializer@221abac1
 此时链是 headHandler，ServerBootstrap$1，io.netty.bootstrap.ServerBootstrap$ServerBootstrapAcceptor@46f3978d，tailhandler
 
 执行完pipeline.remove(this);删除了ServerBootstrap$1 handler，然后只剩下
 headHandler，io.netty.bootstrap.ServerBootstrap$ServerBootstrapAcceptor@46f3978d，tailhandler；
 
  pipeline.fireChannelRegistered(); 比较重要
 
 上文已表，pipeline内部主要在维护DefaultChannelHandlerContext链表，而每个DefaultChannelHandlerContext与ChannelHandler实例相对应。	
                    
 
 ctx.fireChannelRegistered(); 然后跳过ServerBootstrapAcceptor这个handler，找到tailHandler，


ChannelHandlerAdapter实现的ChannelHandler接口的方法都是被@Skip忽视了，所以说，只会执行被Handler子类明确覆盖的方法

这个Pattern设计比较巧妙，首先应该明确执行各个Handler链中的方法；然后如果方法被@Skip注解了，那么该方法则不会执行。

把前一个future作为下一个调用方法的参数，这样可以先判断后再处理，从而提升性能。


	private ChannelFuture doBind(final SocketAddress localAddress) {
        final ChannelFuture regFuture = initAndRegister();
        final Channel channel = regFuture.channel();
        if (regFuture.cause() != null) {
            return regFuture;
        }

        final ChannelPromise promise;
        if (regFuture.isDone()) {
            promise = channel.newPromise();
            doBind0(regFuture, channel, localAddress, promise);
        } else {
            // Registration future is almost always fulfilled already, but just in case it's not.
            promise = new DefaultChannelPromise(channel, GlobalEventExecutor.INSTANCE);
            regFuture.addListener(new ChannelFutureListener() {
                @Override
                public void operationComplete(ChannelFuture future) throws Exception {
                    doBind0(regFuture, channel, localAddress, promise);
                }
            });
        }

        return promise;
    }
 
 
 doBind0 -->  channel.eventLoop().execute --> channel.bind--> pipeline.bind-->DefaultChannelHandlerContext.bind -->DefaultChannelHandlerInvoker.invokeBind--》unsafe.bind(localAddress, promise);-->AbstractChannel/NioServerSocketChannel.doBind(SocketAddress localAddress) throws Exception

 
 魔法在这里？但是为什么要顺序反过来呢？ 一个是ctx = ctx.next;另一个是  ctx = ctx.prev;

 
 
  	private DefaultChannelHandlerContext findContextInbound(int mask) {
        DefaultChannelHandlerContext ctx = this;
        do {
            ctx = ctx.next;
        } while ((ctx.skipFlags & mask) != 0);
        return ctx;
    }

    private DefaultChannelHandlerContext findContextOutbound(int mask) {
        DefaultChannelHandlerContext ctx = this;
        do {
            ctx = ctx.prev;
        } while ((ctx.skipFlags & mask) != 0);
        return ctx;
    }
    
    开始`NioServerSocketChannel`对象创建
至此，完成`NioServerSocketChannel`对象创建。可以看到，创建了javaChannel，设置了是否blocking，初始化了连接参数。

 
## 客户端发送数据

---

## 服务端处理数据

---

## 服务端停止服务

---

## 客户端停止服务

---

## 好的设计
`InternalLogger`用来避免对第三方日志框架的依赖，如slf，log4j等等。
`ChannelOption`灵活地使用泛型机制，避免用户设置参数发生低级错误。
Context 静态的？,Session 动态的？区别
DefaultChannelHandlerContext 设计也很有意思 
ChannelHandlerContext 通过继承AttributeMap 实现有状态，但是里面又实现了firexxx 感觉职责不太清晰 。应该专门抽出 事件管理机制，
@Sharable表示该类是无状态的，仅仅起“文档”标记作用。
@Skip 
在子接口里仅仅把父接口方法返回值覆写了，然后什么都不做。
ChannelSink sunk source这种geek的命名，受不了。 新版本果然悄悄去了，哈哈。
3，4 对比，自己的理解
另外，可以观察到，在子接口里仅仅把父接口方法返回值覆写了，然后什么都不做。这样一定程度上避免了强制转型的尴尬。
谁负责整体框架，领导喊口号，定规则
谁负责具体执行，码农苦执行，
SystemPropertyUtil
String.format("nThreads: %d (expected: > 0)", nThreads)
DefaultThreadFactory
isAssignableFrom？？？
io.netty.util.NetUtil
所有耗时较大的步骤全部异步了。
DefaultChannelHandlerContext 设计精髓，支持多个事件？？
把前一个future作为下一个调用方法的参数，这样可以先判断后再处理，从而提升性能。

主要值得一提的就是channel方法的设计。传递class，然后通过反射来实例化具体的Channel实例,一定程度上避免了写死类名字符串导致未来版本变动时发生错误的可能性。
IdentityHashMap，            // Not using ConcurrentHashMap due to high memory consumption. 消耗内存过大
归纳，演绎 一般和特殊，整体和局部 不完全归纳
往nio的本质上靠，新增了哪些东西。有点像看ORM源码。如何对JDBC封装。
pipeline里面并不直接是handler，需要修改。
## 个人觉得不太好的

b.bind(port)这个里面的内容非常复杂，不仅仅是bind一个port那么简单。所以该方法命名不是很好。

父接口依赖子接口，也不是很好。
DefaultChannelPipeline.addLast 这个方法太坑了，并不是把handler加到最后一个上面。
无一类外的是，继承体系相对复杂。父类，子类的命名通常不能体现出谁是父类，谁是子类，除了一个Abstract能够直接看出来。
## 中立 

addLast0 私有
getter、setter省略？

## 疑问
为什么boss也要是个线程池？作用是啥？高并发下，线程处理不过来，会如何处理？

NioEventLoop.run()是一个死循环？




---

## 注意事项
客户端单实例，防止消耗过多线程。这个在[http://hellojava.info/](http://hellojava.info/)中多次提到。

---

## 参考
[netty 那点事](https://github.com/code4craft/netty-learning)

[官网的相关文章](http://netty.io/wiki/related-articles.html)

[User Guide For 5.x](http://netty.io/wiki/user-guide-for-5.x.html)

{% include JB/setup %}
