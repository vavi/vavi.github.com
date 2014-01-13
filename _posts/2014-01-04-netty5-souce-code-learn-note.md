---
layout: post
title: "netty5源码分析"
description: ""
category: 开源项目
tags: [netty]
---

## Netty是什么
通用的协议或者实现有时不能满足各种各样的需求。就好像，我们通常不会使用一个HTTP Server来同时进行传输大文件，email以及近实时的消息如金融信息和多玩家游戏数据。我们需要一个高度优化的协议实现，来满足一些特定的需求。

Netty是一个**异步，事件驱动**的网络应用框架，并且提供了一些工具来帮助迅速地开发**高性能，高扩展性**的服务端和客户端。

---
  
## 核心领域概念介绍
### 背景

下面的介绍不成系统，有点离散，请各位看官先有个初步概念，如果有问题，可以先翻下UserGuide。

`ChannelHandlerAdapter` 实现了`ChannelHandler`接口 ，`ChannelInitializer`继承了`ChannelHandlerAdapter`。`ChannelHandlerAdapter`大部分职责委托给`ChannelHandlerContext`实现类

`ByteBuf`是一个引用计数对象，必须显式地调用`release()`方法来减少引用计数。  
   
Boss线程池用来接受客户端的连接请求，Worker线程池用来处理boss线程池里面的连接的数据。
   
`ChannelInitializer`是一个特殊的handler，用来初始化ChannelPipeline里面的handler链。 这个特殊的`ChannelInitializer`在加入到pipeline后，在initChannel调用结束后,自身会被remove掉，从而完成初始化的效果（后文会详述）。

`AbstractBootstrap.option()`用来设置ServerSocket的参数，`AbstractBootstrap.childOption()`用来设置Socket的参数。
 
在调用ctx.write(Object)后需要调用ctx.flush()方法，这样才能将数据发出去。或者直接调用 ctx.writeAndFlush(msg)方法。
 
通常使用这种方式来实例化ByteBuf：`final ByteBuf time = ctx.alloc().buffer(4); ` ，而不是直接使用ByteBuf子类的构造方法

`ChannelFuture`表示实际的I/O操作还未发生。因为在netty中，所有的I/O操作（如read, write, connect,和bind）都是异步的。 
 
另外，还需要在处理基于流的传输协议TCP/IP的数据时，注意报文和业务程序实际能够接收到的数据之间的关系。 假如你发送了2个报文，底层是发送了两组字节。但是操作系统的TCP栈是有缓存的，它可能把这两组字节合并成一组字节，然后再给业务程序使用。但是业务程序往往需要根据把这一组字节还原成原来的两组字节，但是不幸的是，业务程序往往无法直接还原，除非在报文上做了些特殊的约定。比如报文是定长的或者有明确的分隔符。  
 
---

### 核心类说明
#### 隐喻
Channel 公路，双向道，分为去路和来路。隐喻为网络连接。 

ChannelPipeline，公路管理部门，可以设置收费站，关卡等设施对车辆进行检查。

ByteBuf 车辆。隐喻为网络连接中的数据。
 
Handler 收费站，关卡；可以对公路上的车辆进行各种处理。

#### Channel
顶层接口，因为有不同的通讯协议，比如TCP、UDP、rxtx等等以及不同的通讯模型，如OIO,NIO,AIO；所以针对这些不同的实现来提供一个统一的接口，在必要的时候，进行向下转型（Downcast），方便扩展。

该接口提供如下功能： 
当前channel的状态（比如是否open，connected）
当前channel的配置参数（比如receive buffer size）
当前channel支持的操作（比如read, write, connect, and bind）

另外，仍然需要值的一提是，Netty中的所有I/O操作都是异步的。这意味着这些I/O方法被调用时，会立即返回`ChannelFuture`，并不保证这些I/O操作实际完成（比如将数据已经发送到对端）。`ChannelFuture`会告诉你这个I/O操作结果是成功，失败还是取消。
 
`Channel`有一个parent。比如，`SocketChannel`是`ServerSocketChannel`的"accept方法"的返回值（ServerSocketChannel中并没有accept方法，但是在ServerSocket中是存在的）。所以，`SocketChannel`的 parent()返回`ServerSocketChannel`。

#### ChannelPipeline
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

#### EventLoopGroup 和 EventLoop
`JUC.ScheduledExecutorService`<--`EventExecutorGroup`<--`EventLoopGroup`<--`EventLoop`
这个"<--"表示是符号的右边继承符号的左边接口。

`JUC.ScheduledExecutorService`是JUC框架的线程调度框架。

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

### NioEventLoopGroup初始化
`NioEventLoopGroup`初始化父类`MultithreadEventLoopGroup`,触发父类获得默认的线程数，其值默认是`Runtime.getRuntime().availableProcessors() * 2`

	static {
        DEFAULT_EVENT_LOOP_THREADS = Math.max(1, SystemPropertyUtil.getInt(
                "io.netty.eventLoopThreads", Runtime.getRuntime().availableProcessors() * 2));

        if (logger.isDebugEnabled()) {
            logger.debug("-Dio.netty.eventLoopThreads: {}", DEFAULT_EVENT_LOOP_THREADS);
        }
    }


接着调用`NioEventLoopGroup`的构造器，
  
   	public NioEventLoopGroup() {
        this(0);
    }
    
   	public NioEventLoopGroup(int nThreads, Executor executor) {
        this(nThreads, executor, SelectorProvider.provider());
    }
    
   	public NioEventLoopGroup(int nThreads) {
        this(nThreads, (Executor) null);
    }

   	public NioEventLoopGroup(int nThreads, ThreadFactory threadFactory) {
        this(nThreads, threadFactory, SelectorProvider.provider());
    }
    
    public NioEventLoopGroup(
            int nThreads, ThreadFactory threadFactory, final SelectorProvider selectorProvider) {
        super(nThreads, threadFactory, selectorProvider);
    }
    
   调用父类MultithreadEventLoopGroup的构造器，该构造器又调用了父类构造器。
    
     	protected MultithreadEventLoopGroup(int nThreads, Executor executor, Object... args) {
        super(nThreads == 0 ? DEFAULT_EVENT_LOOP_THREADS : nThreads, executor, args);
    }

 下面的构造方法主要完成以下几件事情：

 1. 设置默认DefaultThreadFactory线程工厂，主要做了2件事，设置线程池名称和线程名称
 2. 初始化children数组，然后通过调用`NioEventLoopGroup.newChild`方法完成child属性设置。 

 
     	protected MultithreadEventExecutorGroup(int nThreads, Executor executor, Object... args) {
        if (nThreads <= 0) {
            throw new IllegalArgumentException(String.format("nThreads: %d (expected: > 0)", nThreads));
        }

        if (executor == null) {
            executor = new ThreadPerTaskExecutor(newDefaultThreadFactory());
        }

        children = new EventExecutor[nThreads];
        for (int i = 0; i < nThreads; i ++) {
            boolean success = false;
            try {
                children[i] = newChild(executor, args);//重点介绍
                success = true;
            } catch (Exception e) {
                // TODO: Think about if this is a good exception type
                throw new IllegalStateException("failed to create a child event loop", e);
            } finally {
                if (!success) {
                    for (int j = 0; j < i; j ++) {
                        children[j].shutdownGracefully();
                    }

                    for (int j = 0; j < i; j ++) {
                        EventExecutor e = children[j];
                        try {
                            while (!e.isTerminated()) {
                                e.awaitTermination(Integer.MAX_VALUE, TimeUnit.SECONDS);
                            }
                        } catch (InterruptedException interrupted) {
                            Thread.currentThread().interrupt();
                            break;
                        }
                    }
                }
            }
        }

    在newChild方法中，主要完成构建NioEventLoop实例
    
 	 	@Override
    	protected EventLoop newChild(Executor executor, Object... args) throws 	Exception {
        return new NioEventLoop(this, executor, (SelectorProvider) args[0]);
   	 	}
    
    下面的`super(parent, executor, false);`主要是设置NioEventLoopGroup是NioEventLoop的parent。然后调用`openSelector()`创建Selector对象。
    
    	NioEventLoop(NioEventLoopGroup parent, Executor executor, SelectorProvider selectorProvider) {
        super(parent, executor, false);
        if (selectorProvider == null) {
            throw new NullPointerException("selectorProvider");
        }
        provider = selectorProvider;
        selector = openSelector();
    	}
    	

首先，先初始化Selector对象，然后再初始化`SelectedSelectionKeySet`，设置其属性` keysA = new SelectionKey[1024]; keysB = keysA.clone();`。进行了一个优化，设置了`sun.nio.ch.SelectorImpl`的`selectedKeys`和`publicSelectedKeys`属性。用意何在？估计要看下提交记录才可以。TODO
			 	     
      private Selector NioEventLoop.openSelector() {
        final Selector selector;
        try {
            selector = provider.openSelector();
        } catch (IOException e) {
            throw new ChannelException("failed to open a new selector", e);
        }

        if (DISABLE_KEYSET_OPTIMIZATION) {
            return selector;
        }

        try {
            SelectedSelectionKeySet selectedKeySet = new SelectedSelectionKeySet();

            Class<?> selectorImplClass =
                    Class.forName("sun.nio.ch.SelectorImpl", false, ClassLoader.getSystemClassLoader());

            // Ensure the current selector implementation is what we can instrument.
            if (!selectorImplClass.isAssignableFrom(selector.getClass())) {
                return selector;
            }

            Field selectedKeysField = selectorImplClass.getDeclaredField("selectedKeys");
            Field publicSelectedKeysField = selectorImplClass.getDeclaredField("publicSelectedKeys");

            selectedKeysField.setAccessible(true);
            publicSelectedKeysField.setAccessible(true);

            selectedKeysField.set(selector, selectedKeySet);
            publicSelectedKeysField.set(selector, selectedKeySet);

            selectedKeys = selectedKeySet;
            logger.trace("Instrumented an optimized java.util.Set into: {}", selector);
        } catch (Throwable t) {
            selectedKeys = null;
            logger.trace("Failed to instrument an optimized java.util.Set into: {}", selector, t);
        }

        return selector;
    }
    

最后循环完成children数组的初始化` children[i] = newChild(executor, args);`，进而完成`NioEventLoopGroup`对象初始化。   


小结：此时结合Eclipse的DEBUG视图，观察bossGroup的属性，可以基本看到完成如下几个事情

* 设置默认线程数和默认线程工厂
* 通过循环，给每个`NioEventLoop`设置了selector属性，新建了多个Selector对象。

---

### ServerBootstrap 初始化

	ServerBootstrap b = new ServerBootstrap();
    b.group(bossGroup, workerGroup)
    .channel(NioServerSocketChannel.class)
    .childHandler(new TelnetServerInitializer());	
上面这段代码内涵平平，主要设置group属性是bossGroup，childGroup属性是workerGroup。
没啥其他复杂属性赋值。主要值得一提的就是channel方法的设计，通过传递class对象，然后通过反射来实例化具体的Channel实例。

`b
 .bind(port)
 .sync()
 .channel()
 .closeFuture()
 .sync();`，这个方法内容很多，详见下述分析。

b.bind(port)方法会调用下面的doBind方法，在doBind方法中会完成Channel的初始化和绑定端口。有2个方法需要重点介绍，分别是 重点介绍1 和 重点介绍2

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
 
重点介绍1  initAndRegister，里面完成Channel实例创建，实例化和注册channel到selector上。
   
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
   
   重点介绍1.1,调用ServerBootstrap.createChannel() ，通过反射完成Channel实例创建
   
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

        p.addLast(new ChannelInitializer<Channel>() {//重点介绍1.2.1
            @Override
            public void initChannel(Channel ch) throws Exception {
                ch.pipeline().addLast(new ServerBootstrapAcceptor(currentChildHandler, currentChildOptions,
                        currentChildAttrs));
            }
        });
    }


重点介绍1.2.1中，此时pipeline中又多了一个handler：内部类ServerBootstrap$1，此时数组的链表情况如下：HeadHandler，ServerBootstrap$1和TailHandler。另外，再额外吐槽一句，`p.addLast`方法并不是把ServerBootstrap$1放到tail上，而是放到tail的前一个节点上。所以，这个addLast方法命名很是误解。

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
     
  重点介绍2.1，该方法内部有调用了pipeline的方法了（在重点介绍1.3.1.2 中也出现了pipeline调用）。 好吧，是时候介绍pipeline了。 
  
   public ChannelFuture bind(SocketAddress localAddress, ChannelPromise promise) 	{
        return pipeline.bind(localAddress, promise);//重点介绍2.1.1
    } 

#### ChannelPipeline

`DefaultChannelPipeline`是`ChannelPipeline`的实现类，`DefaultChannelPipeline`内部维护了两个指针：`final DefaultChannelHandlerContext head; final DefaultChannelHandlerContext tail;`，分别指向链表的头部和尾部；而`DefaultChannelHandlerContext`内部是一个链表结构：`volatile DefaultChannelHandlerContext next;volatile DefaultChannelHandlerContext prev;`，而每个`DefaultChannelHandlerContext`与`ChannelHandler`实例一一对应。

从上面可以看到，这是个经典的Intercepting Filter模式实现。下面我们再接着从重点介绍1.3.1.2代码看起，`pipeline.fireChannelRegistered();`依次执行如下两个方法。上文也已经说明，此时handler链是HeadHandler，ServerBootstrap$1和TailHandler。
 
 	   @Override
    public ChannelPipeline DefaultChannelPipeline.fireChannelRegistered() {
        head.fireChannelRegistered();
        return this;
    }

	public ChannelHandlerContext ChannelHandlerContext.fireChannelRegistered() {
        DefaultChannelHandlerContext next = findContextInbound(MASK_CHANNEL_REGISTERED); //重点介绍 1.3.1.2.1
        next.invoker.invokeChannelRegistered(next); //重点介绍1.3.1.2.2

        return this;
    }

	private DefaultChannelHandlerContext DefaultChannelHandlerContext.findContextInbound(int mask) {
        DefaultChannelHandlerContext ctx = this;
        do {
            ctx = ctx.next;
        } while ((ctx.skipFlags & mask) != 0);
        return ctx;
    }
    
 重点介绍 1.3.1.2.1,针对这个findContextInbound方法需要再补充下，里面ServerBootstrap$1是继承自ChannelInitializer，而`ChannelInitializer.channelRegistered`是没有@Skip注解的。呃，@Skip注解又有何用。这个要结合`DefaultChannelHandlerContext.skipFlags0(Class<? extends ChannelHandler> handlerType)`。这个skipFlags0方法返回一个整数，如果该方法上标记了@Skip注解，那么表示该方法在Handler被执行时，需要被忽略。所以，此时`do {ctx = ctx.next;} while ((ctx.skipFlags & mask) != 0);`片段的执行结果返回的是ServerBootstrap$1这个Handler。 
  
这里在额外说一句，这个`ChannelHandlerAdapter`里面的方法几乎都被加了@Skip标签。
  
  	private static int skipFlags0(Class<? extends ChannelHandler> handlerType) {
        int flags = 0;
        try {
            if (handlerType.getMethod(
                    "handlerAdded", ChannelHandlerContext.class).isAnnotationPresent(Skip.class)) {
                flags |= MASK_HANDLER_ADDED;
            }
            if (handlerType.getMethod(
                    "handlerRemoved", ChannelHandlerContext.class).isAnnotationPresent(Skip.class)) {
                flags |= MASK_HANDLER_REMOVED;
            }
            if (handlerType.getMethod(
                    "exceptionCaught", ChannelHandlerContext.class, Throwable.class).isAnnotationPresent(Skip.class)) {
                flags |= MASK_EXCEPTION_CAUGHT;
            }
            if (handlerType.getMethod(
                    "channelRegistered", ChannelHandlerContext.class).isAnnotationPresent(Skip.class)) {
                flags |= MASK_CHANNEL_REGISTERED;
            }
            if (handlerType.getMethod(
                    "channelActive", ChannelHandlerContext.class).isAnnotationPresent(Skip.class)) {
                flags |= MASK_CHANNEL_ACTIVE;
            }
            if (handlerType.getMethod(
                    "channelInactive", ChannelHandlerContext.class).isAnnotationPresent(Skip.class)) {
                flags |= MASK_CHANNEL_INACTIVE;
            }
            if (handlerType.getMethod(
                    "channelRead", ChannelHandlerContext.class, Object.class).isAnnotationPresent(Skip.class)) {
                flags |= MASK_CHANNEL_READ;
            }
            if (handlerType.getMethod(
                    "channelReadComplete", ChannelHandlerContext.class).isAnnotationPresent(Skip.class)) {
                flags |= MASK_CHANNEL_READ_COMPLETE;
            }
            if (handlerType.getMethod(
                    "channelWritabilityChanged", ChannelHandlerContext.class).isAnnotationPresent(Skip.class)) {
                flags |= MASK_CHANNEL_WRITABILITY_CHANGED;
            }
            if (handlerType.getMethod(
                    "userEventTriggered", ChannelHandlerContext.class, Object.class).isAnnotationPresent(Skip.class)) {
                flags |= MASK_USER_EVENT_TRIGGERED;
            }
            if (handlerType.getMethod(
                    "bind", ChannelHandlerContext.class,
                    SocketAddress.class, ChannelPromise.class).isAnnotationPresent(Skip.class)) {
                flags |= MASK_BIND;
            }
            if (handlerType.getMethod(
                    "connect", ChannelHandlerContext.class, SocketAddress.class, SocketAddress.class,
                    ChannelPromise.class).isAnnotationPresent(Skip.class)) {
                flags |= MASK_CONNECT;
            }
            if (handlerType.getMethod(
                    "disconnect", ChannelHandlerContext.class, ChannelPromise.class).isAnnotationPresent(Skip.class)) {
                flags |= MASK_DISCONNECT;
            }
            if (handlerType.getMethod(
                    "close", ChannelHandlerContext.class, ChannelPromise.class).isAnnotationPresent(Skip.class)) {
                flags |= MASK_CLOSE;
            }
            if (handlerType.getMethod(
                    "read", ChannelHandlerContext.class).isAnnotationPresent(Skip.class)) {
                flags |= MASK_READ;
            }
            if (handlerType.getMethod(
                    "write", ChannelHandlerContext.class,
                    Object.class, ChannelPromise.class).isAnnotationPresent(Skip.class)) {
                flags |= MASK_WRITE;

                // flush() is skipped only when write() is also skipped to avoid the situation where
                // flush() is handled by the event loop before write() in staged execution.
                if (handlerType.getMethod(
                        "flush", ChannelHandlerContext.class).isAnnotationPresent(Skip.class)) {
                    flags |= MASK_FLUSH;
                }
            }
        } catch (Exception e) {
            // Should never reach here.
            PlatformDependent.throwException(e);
        }

        return flags;
    }
  
 此时，重点介绍1.3.1.2.1 代码片段执行完毕，现在开始重点介绍1.3.1.2.2 执行。
 
  	@Override
    public void DefaultChannelHandlerInvoker.invokeChannelRegistered(final ChannelHandlerContext ctx) {
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
    
    public static void invokeChannelRegisteredNow(ChannelHandlerContext ctx) {
        try {
            ctx.handler().channelRegistered(ctx);
        } catch (Throwable t) {
            notifyHandlerException(ctx, t);
        }
    }
    
 	由于ServerBootstrap$1(ChannelInitializer<C>)这个类继承了ChannelInitializer，所以会执行了ChannelInitializer.channelRegistered这个方法。
 
 	@Override
    @SuppressWarnings("unchecked")
    public final void ChannelInitializer.channelRegistered(ChannelHandlerContext ctx) throws Exception {
        ChannelPipeline pipeline = ctx.pipeline();
        boolean success = false;
        try {
            initChannel((C) ctx.channel());//重点介绍1.3.1.2.2.1
            pipeline.remove(this);//重点介绍1.3.1.2.2.2
            ctx.fireChannelRegistered();//重点介绍1.3.1.2.2.3
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
    
 在重点介绍1.3.1.2.2.1里，又回调了下面的initChannel方法。该方法把ServerBootstrapAcceptor这个Handler加入到Pipeline中；此时handler链情况如下：HeadHandler，ServerBootstrap$1，ServerBootstrap$ServerBootstrapAcceptor和TailHandler 
 
 	 p.addLast(new ChannelInitializer<Channel>() {
            @Override
            public void initChannel(Channel ch) throws Exception {
                ch.pipeline().addLast(new ServerBootstrapAcceptor(currentChildHandler, currentChildOptions,
                        currentChildAttrs));
            }
        });   

在 重点介绍1.3.1.2.2.2里，通过执行`pipeline.remove(this);`又把ServerBootstrap$1这个Handler给删除了，从而完成初始化的效果。需要提醒的是，ServerBootstrapAcceptor的currentChildHandler属性包含了在客户端代码注册的`TelnetServerInitializer`类。

在重点介绍1.3.1.2.2.3里，通过执行`ctx.fireChannelRegistered();`又找到了下一个handler，

 public ChannelHandlerContext DefaultChannelHandlerContext.fireChannelRegistered() {
        DefaultChannelHandlerContext next = findContextInbound(MASK_CHANNEL_REGISTERED);
        next.invoker.invokeChannelRegistered(next);
        return this;
    }
    
这段逻辑和上述基本一样， findContextInbound内部执行时，会跳过ServerBootstrapAcceptor这个handler，最终找到找到tailHandler，并执行channelRegistered()这个方法。就这样，最终完成了整个 `pipeline.fireChannelRegistered();`执行。

static final class TailHandler extends ChannelHandlerAdapter {

    @Override
    public void channelRegistered(ChannelHandlerContext ctx) throws Exception {}
       
    //省略下面的方法 
}           

下面我们再趁热打铁，回头看看 重点介绍2.1代码的执行逻辑。

	 public ChannelFuture AbstractChannel.bind(SocketAddress localAddress, ChannelPromise promise) 	{
        return pipeline.bind(localAddress, promise);//重点介绍2.1.1
    } 
    
    
     @Override
    public ChannelFuture DefaultChannelPipeline.bind(SocketAddress localAddress, ChannelPromise promise) {
        return pipeline.bind(localAddress, promise);
    }
    
     @Override
    public ChannelFuture bind(SocketAddress localAddress, ChannelPromise promise) {
        return tail.bind(localAddress, promise); //重点介绍2.1.1.1
    }
    
  重点介绍2.1.1.1，执行到这里，发现是tail.bind，而不是head.bind。  
    
     @Override
    public ChannelFuture bind(final SocketAddress localAddress, final ChannelPromise promise) {
        DefaultChannelHandlerContext next = findContextOutbound(MASK_BIND);
        next.invoker.invokeBind(next, localAddress, promise);
        return promise;
    }

  
   @Override
    public void DefaultChannelHandlerInvokerinvokeBind(
            final ChannelHandlerContext ctx, final SocketAddress localAddress, final ChannelPromise promise) {
        if (localAddress == null) {
            throw new NullPointerException("localAddress");
        }
        validatePromise(ctx, promise, false);

        if (executor.inEventLoop()) {
            invokeBindNow(ctx, localAddress, promise);
        } else {
            safeExecuteOutbound(new Runnable() {
                @Override
                public void run() {
                    invokeBindNow(ctx, localAddress, promise);
                }
            }, promise);
        }
    }
 
 
 	 public static void ChannelHandlerInvokerUtil.invokeBindNow(
            final ChannelHandlerContext ctx, final SocketAddress localAddress, final ChannelPromise promise) {
        try {
            ctx.handler().bind(ctx, localAddress, promise);
        } catch (Throwable t) {
            notifyOutboundHandlerException(t, promise);
        }
    }
    
    @Override
    public void DefaultChannelPipeline.HeadHandler.bind(
                ChannelHandlerContext ctx, SocketAddress localAddress, ChannelPromise promise)
                throws Exception {
            unsafe.bind(localAddress, promise);
        }
        
     @Override
     public final void AbstractChannel.AbstractUnsafe.bind(final SocketAddress localAddress, final ChannelPromise promise) {
            if (!ensureOpen(promise)) {
                return;
            }

            // See: https://github.com/netty/netty/issues/576
            if (!PlatformDependent.isWindows() && !PlatformDependent.isRoot() &&
                Boolean.TRUE.equals(config().getOption(ChannelOption.SO_BROADCAST)) &&
                localAddress instanceof InetSocketAddress &&
                !((InetSocketAddress) localAddress).getAddress().isAnyLocalAddress()) {
                // Warn a user about the fact that a non-root user can't receive a
                // broadcast packet on *nix if the socket is bound on non-wildcard address.
                logger.warn(
                        "A non-root user can't receive a broadcast packet if the socket " +
                        "is not bound to a wildcard address; binding to a non-wildcard " +
                        "address (" + localAddress + ") anyway as requested.");
            }

            boolean wasActive = isActive();
            try {
                doBind(localAddress);//重点介绍2.1.1.1.1
             } catch (Throwable t) {
                promise.setFailure(t);
                closeIfClosed();
                return;
            }
            if (!wasActive && isActive()) {
                invokeLater(new Runnable() {//重点介绍2.1.1.1.2
                    @Override
                    public void run() {
                        pipeline.fireChannelActive();//重点介绍2.1.1.1.3
                    }
                });
            }
            promise.setSuccess();
        }   
        
   在重点介绍2.1.1.1.1里，执行真正的bind端口作用。
    
    protected void doBind(SocketAddress localAddress) throws Exception {
        javaChannel().socket().bind(localAddress, config.getBacklog());
    }    
 
 	在重点介绍2.1.1.1.2里，执行如下方法，`eventLoop().execute(task); `在下个章节会继续涉及。现在暂时忽略。	
 	private void invokeLater(Runnable task) {
            // This method is used by outbound operation implementations to trigger an inbound event later.
            // They do not trigger an inbound event immediately because an outbound operation might have been
            // triggered by another inbound event handler method.  If fired immediately, the call stack
            // will look like this for example:
            //
            //   handlerA.inboundBufferUpdated() - (1) an inbound handler method closes a connection.
            //   -> handlerA.ctx.close()
            //      -> channel.unsafe.close()
            //         -> handlerA.channelInactive() - (2) another inbound handler method called while in (1) yet
            //
            // which means the execution of two inbound handler methods of the same handler overlap undesirably.
            eventLoop().execute(task);
        }
        
    
  在重点介绍2.1.1.1.3里，执行  `pipeline.fireChannelActive();`方法。  
  	  public ChannelPipeline fireChannelActive() {
        head.fireChannelActive();//重点介绍2.1.1.1.3.1

        if (channel.config().isAutoRead()) {
            channel.read();//重点介绍2.1.1.1.3.2
        }

        return this;
    }
  
  在重点介绍2.1.1.1.3.1里，和上述逻辑一样，最终执行到TailHandler这里。

   static final class TailHandler extends ChannelHandlerAdapter {

        @Override
        public void channelRegistered(ChannelHandlerContext ctx) throws Exception { }

        @Override
        public void channelActive(ChannelHandlerContext ctx) throws Exception { }
        
        //下省略方法
 } 
 
在重点介绍2.1.1.1.3.2里，由于channel.config().isAutoRead()默认返回true；
 	     
     @Override
    public ChannelPipeline read() {
        tail.read();
        return this;
    }
    
     @Override
        public void DefaultChannelPipeline.HeadHandler.read(ChannelHandlerContext ctx) {
            unsafe.beginRead();
        }
 
 
 	 @Override
     public void AbstractChannel.AbstractUnsafe.beginRead() {
            if (!isActive()) {
                return;
            }

            try {
                doBeginRead();
            } catch (final Exception e) {
                invokeLater(new Runnable() {
                    @Override
                    public void run() {
                        pipeline.fireExceptionCaught(e);
                    }
                });
                close(voidPromise());
            }
        }
        
     protected void AbstractNioChannel.doBeginRead() throws Exception {
        if (inputShutdown) {
            return;
        }

        final SelectionKey selectionKey = this.selectionKey;
        if (!selectionKey.isValid()) {
            return;
        }

        final int interestOps = selectionKey.interestOps();
        if ((interestOps & readInterestOp) == 0) {
            selectionKey.interestOps(interestOps | readInterestOp);
        }
    }   
 
支持整个DefaultPromise.bind方法执行完毕，下面开始执行。
 
 	 @Override
    public Promise<V> DefaultPromise.sync() throws InterruptedException {
        await();
        rethrowIfFailed();
        return this;
    }
    
     @Override
    public Promise<V> DefaultPromise.await() throws InterruptedException {
        if (isDone()) {
            return this;
        }

        if (Thread.interrupted()) {
            throw new InterruptedException(toString());
        }

        synchronized (this) {
            while (!isDone()) {
                checkDeadLock();
                incWaiters();
                try {
                    wait();
                } finally {
                    decWaiters();
                }
            }
        }
        return this;
    }
 
#### channel.eventLoop().execute 
 
  
 
5. 在`AbstractBootstrap.doBind0()`中调用了`channel.bind(localAddress, promise).addListener(ChannelFutureListener.CLOSE_ON_FAILURE);`方法，完成了bind具体的服务器端口，最终调用了`NioServerSocketChannel.doBind(SocketAddress localAddress)`完成bind。接着触发`pipeline.fireChannelActive()`事件。

invokeChannelRegisteredNow 静态导入

 此时完成XX方法执行，现在回到XXX方法
  

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

总结下服务端模式 


--- 

## 客户端发送数据

客户端和服务端比较相似

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

   1. 对象继承关系如下，`JUC.AbstractExecutorService`<--`AbstractEventExecutor`<--`SingleThreadEventExecutor`<--`SingleThreadEventLoop`<--`NioEventLoop`
		

主要值得一提的就是channel方法的设计。传递class，然后通过反射来实例化具体的Channel实例,一定程度上避免了写死类名字符串导致未来版本变动时发生错误的可能性。
IdentityHashMap，            // Not using ConcurrentHashMap due to high memory consumption. 消耗内存过大
归纳，演绎 一般和特殊，整体和局部 不完全归纳
往nio的本质上靠，新增了哪些东西。有点像看ORM源码。如何对JDBC封装。
pipeline里面并不直接是handler，需要修改。

Env，Context，Session

LoopGroup和ExcutorGroup相当于Loop和Excutor的容器，Group中包括了多个Loop和多个Excutor，所以单个Loop和Excutor也可以理解为一个Group，但其中只有一个Loop和Excutor。Loop用于事件循环，Excutor用于任务的提交调度执行。

 背景介绍

1. `Channel`<--`AbstractChannel`<--`AbstractNioChannel`<--`AbstractNioMessageChannel`<--`AbstractNioMessageServerChannel`<--`NioServerSocketChannel	`，类继承关系如上，相对比较清晰。
2. `Unsafe`<--`NioUnsafe`<--`AbstractNioUnsafe`<--`NioMessageUnsafe`。在实现上，`AbstractUnsafe`是`AbstractChannel`的内部类。

另外，内部还有name2ctx这个map属性，也就是说，这个类既提供了O(N)，也提供O(1)操作。
	
框架帮趟坑，然后在类加载时，主要涉及到对象的初始化。其中一个是在`NioEventLoop`的静态块中解决了JDK 的一个bug ；buildSelector bug	
	
当我们跳出里面的细节时，考虑一下，你是作者的话，会如何考虑。整体的一个算法 。
 nio，sun jdk bug, option(默认和用户设置)，异步future、executor，pipeline、context、handler，nio，设计模式（模板），不同的通信，tcp，udp，
 
 TailHandler处理inbound类型的数据；HeadHandler处理outbound类型的数据。?TailHandler的实现函数都是空的，这说明对于底层上来应用的数据，用户必须定义Handler来处理，不能使用默认的Handler进行处理。HeaderHandler的实现函数都是基于unsafe对象的函数实现的，所以对于OutBound类型的数据，即应用往底层的数据，可以使用默认的Handler进行处理。

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

[Core J2EE Patterns - Intercepting Filter](http://www.oracle.com/technetwork/java/interceptingfilter-142169.html)


{% include JB/setup %}
