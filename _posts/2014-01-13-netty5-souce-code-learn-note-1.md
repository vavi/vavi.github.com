---
layout: post
title: "Netty5源码分析--1.服务端启动过程详解"
description: ""
category: 开源项目
tags: [netty]

---

## 实例
样例代码来自于`io.netty.example.telnet.TelnetServer`，完整样例请参考NettyExample工程。
	
	public class TelnetServer {

    private final int port;

    public TelnetServer(int port) {
        this.port = port;
    }

    public void run() throws Exception {
        EventLoopGroup bossGroup = new NioEventLoopGroup();//bossGroup线程池用来接受客户端的连接请求
        EventLoopGroup workerGroup = new NioEventLoopGroup();//workerGroup线程池用来处理boss线程池里面的连接的数据
        try {
            ServerBootstrap b = new ServerBootstrap();
            b.group(bossGroup, workerGroup)
             .channel(NioServerSocketChannel.class)
             .childHandler(new TelnetServerInitializer());//ChannelInitializer是一个特殊的handler，用来初始化ChannelPipeline里面的handler链。 这个特殊的ChannelInitializer在加入到pipeline后，在initChannel调用结束后,自身会被remove掉，从而完成初始化的效果（后文会详述）。

	//AbstractBootstrap.option()用来设置ServerSocket的参数，AbstractBootstrap.childOption()用来设置Socket的参数。

            b.bind(port).sync().channel().closeFuture().sync();
        } finally {
            bossGroup.shutdownGracefully();
            workerGroup.shutdownGracefully();
        }
    }

    public static void main(String[] args) throws Exception {
        int port;
        if (args.length > 0) {
            port = Integer.parseInt(args[0]);
        } else {
            port = 8080;
        }
        new TelnetServer(port).run();
    }
 	}

针对上述代码，还需要补充介绍一些内容：

在调用ctx.write(Object)后需要调用ctx.flush()方法，这样才能将数据发出去。或者直接调用 ctx.writeAndFlush(msg)方法。
 
通常使用这种方式来实例化ByteBuf：`final ByteBuf time = ctx.alloc().buffer(4); ` ，而不是直接使用ByteBuf子类的构造方法
 
另外，还需要在处理基于流的传输协议TCP/IP的数据时，注意报文和业务程序实际能够接收到的数据之间的关系。 假如你发送了2个报文，底层是发送了两组字节。但是操作系统的TCP栈是有缓存的，它可能把这两组字节合并成一组字节，然后再给业务程序使用。但是业务程序往往需要根据把这一组字节还原成原来的两组字节，但是不幸的是，业务程序往往无法直接还原，除非在报文上做了些特殊的约定。比如报文是定长的或者有明确的分隔符。 

## 服务端启动服务
当`TelnetServer启动时，依次完成如下步骤：

### NioEventLoopGroup初始化

当`NioEventLoopGroup`构造方法被调用时，首先初始化父类`MultithreadEventLoopGroup`,触发父类获得默认的线程数，其值默认是`Runtime.getRuntime().availableProcessors() * 2`

	static {
        DEFAULT_EVENT_LOOP_THREADS = Math.max(1, SystemPropertyUtil.getInt(
                "io.netty.eventLoopThreads", Runtime.getRuntime().availableProcessors() * 2));

        if (logger.isDebugEnabled()) {
            logger.debug("-Dio.netty.eventLoopThreads: {}", DEFAULT_EVENT_LOOP_THREADS);
        }
    }


接着调用`NioEventLoopGroup`自身的构造器，依次执行下面的构造器。
  
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
    
 继续调用父类MultithreadEventLoopGroup的构造器，该构造器又调用了父类构造器。
    
     	protected MultithreadEventLoopGroup(int nThreads, Executor executor, Object... args) 	{
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
                children[i] = newChild(executor, args);//tag
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
    	

首先，先初始化Selector对象，然后再初始化`SelectedSelectionKeySet`，设置其属性` keysA = new SelectionKey[1024]; keysB = keysA.clone();`。进行了一个优化，设置了`sun.nio.ch.SelectorImpl`的`selectedKeys`和`publicSelectedKeys`属性。根据NioEventLoop.run()方法内部直接调用 `processSelectedKeysOptimized(selectedKeys.flip());`并且没有直接使用`selector.selectedKeys()`这两处代码，笔者猜测正是因为在此时通过反射设置了属性，所以NioEventLoop.run()才能正常工作。
			 	     
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


#### 小结

此时再结合Eclipse的DEBUG视图，观察bossGroup的属性，可以基本看到完成如下几个事情

* 创建NioEventLoopGroup对象
* 获得默认线程池数目大小，数值为N
* 设置线程池名称和线程名称
* 循环创建出来 N个NioEventLoop对象，每个NioEventLoop都设置了相同的parent，executor和不同的selector实例。

---

### ServerBootstrap 初始化

	ServerBootstrap b = new ServerBootstrap();
    	
上面这段代码内涵平平，主要设置group属性是bossGroup，childGroup属性是workerGroup。
没啥其他复杂属性赋值。主要值得一提的就是channel方法的设计，通过传递class对象，然后通过反射来实例化具体的Channel实例。

`b
 .bind(port)
 .sync()
 .channel()
 .closeFuture()
 .sync();`，这个方法的内容很多，详见下述分析。

b.bind(port)方法会调用下面的doBind方法，在doBind方法中会完成Channel的初始化和绑定端口。有2个方法需要tag，分别是 tag1 和 tag2

    private ChannelFuture doBind(final SocketAddress localAddress) {
        final ChannelFuture regFuture = initAndRegister();//tag1
        final Channel channel = regFuture.channel();
        if (regFuture.cause() != null) {
            return regFuture;
        }

        final ChannelPromise promise;
        if (regFuture.isDone()) {
            promise = channel.newPromise();
            doBind0(regFuture, channel, localAddress, promise);//tag2
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
 
tag1  initAndRegister，里面完成Channel实例创建，实例化和注册channel到selector上。
   
    final ChannelFuture initAndRegister() {
        Channel channel;
        try {
            channel = createChannel();//tag1.1
        } catch (Throwable t) {
            return VoidChannel.INSTANCE.newFailedFuture(t);
        }

        try {
            init(channel);//tag1.2
        } catch (Throwable t) {
            channel.unsafe().closeForcibly();
            return channel.newFailedFuture(t);
        }

        ChannelPromise regFuture = channel.newPromise();
        channel.unsafe().register(regFuture);//tag1.3
        if (regFuture.cause() != null) {
            if (channel.isRegistered()) {
                channel.close();
            } else {
                channel.unsafe().closeForcibly();
            }
        }
   
   tag1.1,调用ServerBootstrap.createChannel() ，通过反射完成Channel实例创建。这里使用了childGroup这个属性，即workGroup线程池。
   
    @Override
    Channel createChannel() {
        EventLoop eventLoop = group().next();
        return channelFactory().newChannel(eventLoop, childGroup);//tag1.1.1

    }
    
tag1.1.1，此时将断点打到NioServerSocketChannel的构造方法上     

	 public NioServerSocketChannel(EventLoop eventLoop, EventLoopGroup childGroup) {
        super(null, eventLoop, childGroup, newSocket(), SelectionKey.OP_ACCEPT);//tag1.1.1.1
        config = new DefaultServerSocketChannelConfig(this, javaChannel().socket());//tag1.1.1.2

    }

 tag1.1.1.1,这段代码主要完成3件事。
 
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
        super(parent, eventLoop);//tag1.1.1.1.1
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
 
 tag1.1.1.1.1中，在`AbstractChannel(Channel parent, EventLoop eventLoop)`中，进行了两个重要操作：` unsafe = newUnsafe();pipeline = new DefaultChannelPipeline(this);`。
 
 	protected AbstractChannel(Channel parent, EventLoop eventLoop) {
        this.parent = parent;
        this.eventLoop = validate(eventLoop);
        unsafe = newUnsafe();
        pipeline = new DefaultChannelPipeline(this);//tag1.1.1.1.1.1
    }

tag1.1.1.1.1.1,设置了HeadHandler和TailHandler。这两个类也比较重要。
    
   public DefaultChannelPipeline(AbstractChannel channel) {
        if (channel == null) {
            throw new NullPointerException("channel");
        }
        this.channel = channel;

        TailHandler tailHandler = new TailHandler();
        tail = new DefaultChannelHandlerContext(this, null, generateName(tailHandler), tailHandler);//tag1.1.1.1.1.1.1

        HeadHandler headHandler = new HeadHandler(channel.unsafe());
        head = new DefaultChannelHandlerContext(this, null, generateName(headHandler), headHandler);

        head.next = tail;
        tail.prev = head;
    }  
 
 tag1.1.1.1.1.1.1,这个方法完成了DefaultChannelHandlerContext的对象的初始化。这个类也是核心类，先暂时把它当成个黑盒，会在后面重点分析。
   
   
 此时，我们方法调用栈结束，然后回到 tag1.1.1.2 这段代码上来。 在`DefaultServerSocketChannelConfig`中构造方法中完成了channel的参数设置
 
 至此，才完成tag1.1 AbstractBootstrap.createChannel()方法的执行。现在又开始 tag1.2的代码片段。该  AbstractBootstrap.init(Channel channel)  方法里面主要涉及到Parent Channel 和 Child Channel的option和attribute 设置，并将客户端设置的参数覆盖到默认参数中；最后，还将`childHandler(new TelnetServerInitializer())`中设置的handler加入到pipeline()中。代码见下。
 
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

        p.addLast(new ChannelInitializer<Channel>() {//tag1.2.1
            @Override
            public void initChannel(Channel ch) throws Exception {
                ch.pipeline().addLast(new ServerBootstrapAcceptor(currentChildHandler, currentChildOptions,
                        currentChildAttrs));
            }
        });
    }


tag1.2.1中，此时pipeline中又多了一个handler：内部类ServerBootstrap$1，此时数组的链表情况如下：HeadHandler，ServerBootstrap$1和TailHandler。另外，再额外吐槽一句，`p.addLast`方法并不是把ServerBootstrap$1放到tail上，而是放到tail的前一个节点上。所以，这个addLast方法命名很是误解。

至此完成tag1.2执行，开始执行tag1.3 `channel.unsafe().register(regFuture);`这段代码。该方法内部接着执行执行tag1.3.1的代码。

	public final void register(final ChannelPromise promise) {
            if (eventLoop.inEventLoop()) {
                register0(promise);
            } else {
                try {
                    eventLoop.execute(new Runnable() {
                        @Override
                        public void run() {//tag1.3.1
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

tag1.3.1,该片段主要执行`doRegister();`和` pipeline.fireChannelRegistered();//tag1.3.1.2`


 	private void register0(ChannelPromise promise) {
            try {
                // check if the channel is still open as it could be closed in the mean time when the register
                // call was outside of the eventLoop
                if (!ensureOpen(promise)) {
                    return;
                }
                doRegister();//tag1.3.1.1
                registered = true;
                promise.setSuccess();
                pipeline.fireChannelRegistered();//tag1.3.1.2
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

tag1.3.1.1 将代码片段将javachannel注册到selector上，并把selectionKey属性赋值

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

tag1.3.1.2，这个方法里面有一堆事情要讲。先暂且放下，在后文讲到ChannelPipeline时会再次回来看这段代码。 
 
 public ChannelPipeline DefaultChannelPipeline.fireChannelRegistered() {
        head.fireChannelRegistered();
        return this;
    }

 
    
 此时终于完成 tag1 代码片段执行，开始执行 tag2 的代码片段。  
 
 private static void doBind0(
            final ChannelFuture regFuture, final Channel channel,
            final SocketAddress localAddress, final ChannelPromise promise) {

        // This method is invoked before channelRegistered() is triggered.  Give user handlers a chance to set up
        // the pipeline in its channelRegistered() implementation.
        channel.eventLoop().execute(new Runnable() {
            @Override
            public void run() {
                if (regFuture.isSuccess()) {
                    channel.bind(localAddress, promise).addListener(ChannelFutureListener.CLOSE_ON_FAILURE);//tag2.1
                } else {
                    promise.setFailure(regFuture.cause());
                }
            }
        });
    }
     
  tag2.1，该方法内部有调用了pipeline的方法了（在tag1.3.1.2 中也出现了pipeline调用）。 好吧，是时候介绍pipeline了。 
  
   public ChannelFuture bind(SocketAddress localAddress, ChannelPromise promise) 	{
        return pipeline.bind(localAddress, promise);//tag2.1.1
    } 

#### ChannelPipeline

`DefaultChannelPipeline`是`ChannelPipeline`的实现类，`DefaultChannelPipeline`内部维护了两个指针：`final DefaultChannelHandlerContext head; final DefaultChannelHandlerContext tail;`，分别指向链表的头部和尾部；而`DefaultChannelHandlerContext`内部是一个链表结构：`volatile DefaultChannelHandlerContext next;volatile DefaultChannelHandlerContext prev;`，而每个`DefaultChannelHandlerContext`与`ChannelHandler`实例一一对应。

从上面可以看到，这是个经典的Intercepting Filter模式实现。下面我们再接着从tag1.3.1.2代码看起，`pipeline.fireChannelRegistered();`依次执行如下两个方法。上文也已经说明，此时handler链是HeadHandler，ServerBootstrap$1和TailHandler。
 
 	   @Override
    public ChannelPipeline DefaultChannelPipeline.fireChannelRegistered() {
        head.fireChannelRegistered();
        return this;
    }

	public ChannelHandlerContext ChannelHandlerContext.fireChannelRegistered() {
        DefaultChannelHandlerContext next = findContextInbound(MASK_CHANNEL_REGISTERED); //tag 1.3.1.2.1
        next.invoker.invokeChannelRegistered(next); //tag1.3.1.2.2

        return this;
    }

	private DefaultChannelHandlerContext DefaultChannelHandlerContext.findContextInbound(int mask) {
        DefaultChannelHandlerContext ctx = this;
        do {
            ctx = ctx.next;
        } while ((ctx.skipFlags & mask) != 0);
        return ctx;
    }
    
 tag 1.3.1.2.1,针对这个findContextInbound方法需要再补充下，里面ServerBootstrap$1是继承自ChannelInitializer，而`ChannelInitializer.channelRegistered`是没有@Skip注解的。呃，@Skip注解又有何用。这个要结合`DefaultChannelHandlerContext.skipFlags0(Class<? extends ChannelHandler> handlerType)`。这个skipFlags0方法返回一个整数，如果该方法上标记了@Skip注解，那么表示该方法在Handler被执行时，需要被忽略。所以，此时`do {ctx = ctx.next;} while ((ctx.skipFlags & mask) != 0);`片段的执行结果返回的是ServerBootstrap$1这个Handler。 
  
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
  
 此时，tag1.3.1.2.1 代码片段执行完毕，现在开始tag1.3.1.2.2 执行。
 
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
            initChannel((C) ctx.channel());//tag1.3.1.2.2.1
            pipeline.remove(this);//tag1.3.1.2.2.2
            ctx.fireChannelRegistered();//tag1.3.1.2.2.3
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
    
 在tag1.3.1.2.2.1里，又回调了下面的initChannel方法。该方法把ServerBootstrapAcceptor这个Handler加入到Pipeline中；此时handler链情况如下：HeadHandler，ServerBootstrap$1，ServerBootstrap$ServerBootstrapAcceptor和TailHandler 
 
 	 p.addLast(new ChannelInitializer<Channel>() {
            @Override
            public void initChannel(Channel ch) throws Exception {
                ch.pipeline().addLast(new ServerBootstrapAcceptor(currentChildHandler, currentChildOptions,
                        currentChildAttrs));
            }
        });   

在 tag1.3.1.2.2.2里，通过执行`pipeline.remove(this);`又把ServerBootstrap$1这个Handler给删除了，从而完成初始化的效果。需要提醒的是，ServerBootstrapAcceptor的currentChildHandler属性包含了在客户端代码注册的`TelnetServerInitializer`类。

在tag1.3.1.2.2.3里，通过执行`ctx.fireChannelRegistered();`又找到了下一个handler，

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

下面我们再趁热打铁，回头看看 tag2.1代码的执行逻辑。

	 public ChannelFuture AbstractChannel.bind(SocketAddress localAddress, ChannelPromise promise) 	{
        return pipeline.bind(localAddress, promise);//tag2.1.1
    } 
    
    
     @Override
    public ChannelFuture DefaultChannelPipeline.bind(SocketAddress localAddress, ChannelPromise promise) {
        return pipeline.bind(localAddress, promise);
    }
    
     @Override
    public ChannelFuture bind(SocketAddress localAddress, ChannelPromise promise) {
        return tail.bind(localAddress, promise); //tag2.1.1.1
    }
    
  tag2.1.1.1，执行到这里，发现是tail.bind，而不是head.bind。  
    
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
                doBind(localAddress);//tag2.1.1.1.1
             } catch (Throwable t) {
                promise.setFailure(t);
                closeIfClosed();
                return;
            }
            if (!wasActive && isActive()) {
                invokeLater(new Runnable() {//tag2.1.1.1.2
                    @Override
                    public void run() {
                        pipeline.fireChannelActive();//tag2.1.1.1.3
                    }
                });
            }
            promise.setSuccess();//tag2.1.1.1.4
        }   
        
   在tag2.1.1.1.1里，执行真正的bind端口。
    
    protected void doBind(SocketAddress localAddress) throws Exception {
        javaChannel().socket().bind(localAddress, config.getBacklog());
    }    
 
 	在tag2.1.1.1.2里，执行如下方法，`eventLoop().execute(task); `在后续分析。现在暂时忽略。	
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
 
这里需要说一下，虽然先执行了`invokeLater`该方法，但是仅仅是把给task加入到队列中，然后等 tag2.1.1.1.4 方法执行后，在下一个循环中再继续执行。

	@Override
    public ChannelPromise DefaultChannelPromise.setSuccess() {
        return setSuccess(null);
    }
    
    @Override
    public ChannelPromise setSuccess(Void result) {
        super.setSuccess(result);
        return this;
    }
     
    @Override
    public Promise<V> setSuccess(V result) {
        if (setSuccess0(result)) {// tag2.1.1.1.4.1
            notifyListeners();// tag2.1.1.1.4.2
            return this;
        }
        throw new IllegalStateException("complete already: " + this);
    }
    
     private boolean setSuccess0(V result) {
        if (isDone()) {
            return false;
        }

        synchronized (this) {
            // Allow only once.
            if (isDone()) {
                return false;
            }
            if (result == null) {
                this.result = SUCCESS;// tag2.1.1.1.4.1.1
            } else {
                this.result = result;
            }
            if (hasWaiters()) {
                notifyAll();
            }
        }
        return true;
    }
  
  在 tag2.1.1.1.4.1.1 设置了成功状态，然后该方法返回，继续执行了tag2.1.1.1.4.2方法。由于listeners为 null，所以直接返回。
   
   	private void notifyListeners() {
        
        Object listeners = this.listeners;
        if (listeners == null) {
            return;
        }
 		// 省略XXXXXX
    }  
   
  此时，程序完成了tag2.1 代码执行，开始继续循环。此时执行 tag2.1.1.1.3里的代码，即执行`pipeline.fireChannelActive();`方法。 
   
  	  public ChannelPipeline fireChannelActive() {
        head.fireChannelActive();//tag2.1.1.1.3.1

        if (channel.config().isAutoRead()) {
            channel.read();//tag2.1.1.1.3.2
        }

        return this;
    }
  
  在tag2.1.1.1.3.1里，和上述逻辑一样，最终执行到TailHandler这里。

   static final class TailHandler extends ChannelHandlerAdapter {

        @Override
        public void channelRegistered(ChannelHandlerContext ctx) throws Exception { }

        @Override
        public void channelActive(ChannelHandlerContext ctx) throws Exception { }
        
        //下省略方法
 } 
 
在tag2.1.1.1.3.2里，由于channel.config().isAutoRead()默认返回true；
 	     
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
 
此属性 readInterestOp值为16，interestOps & readInterestOp值为0，所以执行了`selectionKey.interestOps(interestOps | readInterestOp);`，等同于执行了`selectionKey.interestOps(SelectionKey.OP_ACCEPT);`。 
        
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
 
 
 
 至此，整个DefaultPromise.bind方法执行完毕，下面开始执行`DefaultPromise.sync()`。而此时在 tag2.1.1.1.4.1.1 已经将值设为SUCCESS了，所以不需要等待，直接返回。
 
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
    
然后系统接着执行了  `b.bind(port).sync().channel().closeFuture().sync();`的后半截方法“channel().closeFuture().sync()”方法。而由于closeFuture这个属性的执行结果一直没有赋值，所以被wait了，从而一直处于wait状态。
  
至此，主线程处于wait状态，并通过子线程无限循环，来完成客户端请求。

---
  
#### 小结
通过channel方法设置不同的通道类型，通过childHandler设置SocketChannel的Handler链

bind(port)完成的职责很多，远不同于ServerSocket.bind方法。具体包含：initAndRegister和doBind0。

其中initAndRegister又细化了createChannel() 和init(channel)以及channel.unsafe().register(regFuture)这3个大步骤。

* createChannel内部 使用了childGroup，group().next()，ServerSocketChannel.open()这3个属性来创建NioServerSocketChannel实例，并初始化了默认参数DefaultServerSocketChannelConfig和DefaultChannelPipeline对象。DefaultChannelPipeline对象默认包含设置了HeadHandler和TailHandler。然后设置了ch.configureBlocking(false)模式，并将readInterestOp赋值为SelectionKey.OP_ACCEPT。

* init(channel方法里面主要涉及到将Parent Channel 和 Child Channel的option和attribute 设值，并将客户端设置的参数覆盖到默认参数中；最后，还将`childHandler(new TelnetServerInitializer())`中设置的handler加入到pipeline()中。
 
* channel.unsafe().register(regFuture) 把ServerBootstrapAcceptor这个Handler加入到Pipeline中


doBind0方法内部执行了javaChannel().register(eventLoop().selector, 0, this);  触发了服务端的channelActive() 事件，并设置了 `selectionKey.interestOps(SelectionKey.OP_ACCEPT);`

---

## 参考

[Wiki Event loop](http://en.wikipedia.org/wiki/Event_loop)

[Architecture of a Highly Scalable NIO-Based Server](https://today.java.net/pub/a/today/2007/02/13/architecture-of-highly-scalable-nio-server.html)
 
[nio框架中的多个Selector结构](http://www.iteye.com/topic/482269) 
 
[官网的相关文章](http://netty.io/wiki/related-articles.html)

[User Guide For 5.x](http://netty.io/wiki/user-guide-for-5.x.html)

[Core J2EE Patterns - Intercepting Filter](http://www.oracle.com/technetwork/java/interceptingfilter-142169.html)


{% include JB/setup %}
