---

layout: post
title: "Netty5源码分析--2.客户端启动过程"
description: ""
category: 开源项目
tags: [netty]

---
## 实例

样例代码来自于`io.netty.example.telnet.TelnetClient`，完整样例请参考NettyExample工程。

客户端和服务端比较相似，所以本篇会在一定程度上略去重复的部分，以减少篇幅。
 
	public void run() throws Exception {
        EventLoopGroup group = new NioEventLoopGroup();
        try {
            Bootstrap b = new Bootstrap();
            b.group(group)
             .channel(NioSocketChannel.class)
             .handler(new TelnetClientInitializer());

            // Start the connection attempt.
            Channel ch = b.connect(host, port).sync().channel();

            // Read commands from the stdin.
            ChannelFuture lastWriteFuture = null;
            BufferedReader in = new BufferedReader(new InputStreamReader(System.in));
            for (;;) {
                String line = in.readLine();
                if (line == null) {
                    break;
                }

                // Sends the received line to the server.
                lastWriteFuture = ch.writeAndFlush(line + "\r\n");

                // If user typed the 'bye' command, wait until the server closes
                // the connection.
                if ("bye".equals(line.toLowerCase())) {
                    ch.closeFuture().sync();
                    break;
                }
            }

            // Wait until all messages are flushed before closing the channel.
            if (lastWriteFuture != null) {
                lastWriteFuture.sync();
            }
        } finally {
            group.shutdownGracefully();
        }
    }

## 客户端启动


	Bootstrap b = new Bootstrap();  //tag0
    b.group(group) //tag1
    .channel(NioSocketChannel.class) //tag2
    .handler(new TelnetClientInitializer());//tag3


tag0代码主要初始化了父类的 options和attrs 属性；代码略。

tag1设置了group属性
 	
 	@SuppressWarnings("unchecked")
    public B group(EventLoopGroup group) {
        if (group == null) {
            throw new NullPointerException("group");
        }
        if (this.group != null) {
            throw new IllegalStateException("group set already");
        }
        this.group = group;
        return (B) this;
    } 
 tag2设置了channelFactory属性   
 
  	public Bootstrap channel(Class<? extends Channel> channelClass) {
        if (channelClass == null) {
            throw new NullPointerException("channelClass");
        }
        return channelFactory(new BootstrapChannelFactory<Channel>(channelClass));
    } 
  
 tag3设置了handler属性     
     
   public B handler(ChannelHandler handler) {
        if (handler == null) {
            throw new NullPointerException("handler");
        }
        this.handler = handler;
        return (B) this;
    }
    
下面开始第二段代码分析，依次执行下面的方法。
    
    Channel ch = b.connect(host, port) //tag4
    .sync().channel(); //tag5
    
     public ChannelFuture connect(String inetHost, int inetPort) {
        return connect(new InetSocketAddress(inetHost, inetPort));
    }

    public ChannelFuture connect(SocketAddress remoteAddress) {
        if (remoteAddress == null) {
            throw new NullPointerException("remoteAddress");
        }

        validate();
        return doConnect(remoteAddress, localAddress());
    }
    
     private ChannelFuture doConnect(final SocketAddress remoteAddress, final SocketAddress localAddress) {
        final ChannelFuture regFuture = initAndRegister();//tag4.1
        final Channel channel = regFuture.channel();
        if (regFuture.cause() != null) {
            return regFuture;
        }

        final ChannelPromise promise = channel.newPromise();
        if (regFuture.isDone()) {
     doConnect0(regFuture, channel, remoteAddress, localAddress, promise);//tag4.2
        } else {
            regFuture.addListener(new ChannelFutureListener() {
                @Override
                public void operationComplete(ChannelFuture future) throws Exception {
                    doConnect0(regFuture, channel, remoteAddress, localAddress, promise);
                }
            });
        }

        return promise;
    }
    
    分析tag4.1代码，细心的读者注意到，这些和服务端的代码执行过程是一样的。运用模板模式，子类定义独特的实现。
    
    final ChannelFuture AbstractBootstrap.initAndRegister() {
        Channel channel;
        try {
            channel = createChannel();//tag4.1.1

        } catch (Throwable t) {
            return VoidChannel.INSTANCE.newFailedFuture(t);
        }

        try {
            init(channel);//tag4.1.2
        } catch (Throwable t) {
            channel.unsafe().closeForcibly();
            return channel.newFailedFuture(t);
        }

        ChannelPromise regFuture = channel.newPromise();
        channel.unsafe().register(regFuture);//tag4.1.3
        if (regFuture.cause() != null) {
            if (channel.isRegistered()) {
                channel.close();
            } else {
                channel.unsafe().closeForcibly();
            }
        }
  
  分析 tag4.1.1，里面通过反射来实例化NioSocketChannel 
        
        @Override
    Channel createChannel() {
        EventLoop eventLoop = group().next();
        return channelFactory().newChannel(eventLoop);//tag4.1.1.1

    }
    
    public NioSocketChannel(EventLoop eventLoop) {
        this(eventLoop, newSocket());//调用下面的newSocket()方法
    }

	private static SocketChannel newSocket() {
        try {
            return SocketChannel.open();
        } catch (IOException e) {
            throw new ChannelException("Failed to open a socket.", e);
        }
    }
    
     public NioSocketChannel(EventLoop eventLoop, SocketChannel socket) {
        this(null, eventLoop, socket);
    }

	protected AbstractNioByteChannel(Channel parent, EventLoop eventLoop, SelectableChannel ch) {
        super(parent, eventLoop, ch, SelectionKey.OP_READ);//调用父类方法
    }
    
    protected AbstractNioChannel(Channel parent, EventLoop eventLoop, SelectableChannel ch, int readInterestOp) {
        super(parent, eventLoop);//调用父类方法,tag4.1.1.1
        this.ch = ch;
        this.readInterestOp = readInterestOp;
        try {
            ch.configureBlocking(false);//tag4.1.1.2
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

	protected AbstractChannel(Channel parent, EventLoop eventLoop) {
        this.parent = parent;
        this.eventLoop = validate(eventLoop);
        unsafe = newUnsafe();
        pipeline = new DefaultChannelPipeline(this);//tag4.1.1.1.1
    }
 
 分析tag4.1.1.1.1，里面调用DefaultChannelPipeline构造器，和服务端的逻辑一样，故不作分析。
 
 此时系统返回到tag4.1.1.2 继续执行`ch.configureBlocking(false);`，此时完成tag4.1.1 方法执行，开始执行tag4.1.2方法
 
 	 @Override
    @SuppressWarnings("unchecked")
    void init(Channel channel) throws Exception {
        ChannelPipeline p = channel.pipeline();
        p.addLast(handler());//tag4.1.2.1

        final Map<ChannelOption<?>, Object> options = options();
        synchronized (options) {
            for (Entry<ChannelOption<?>, Object> e: options.entrySet()) {
                try {
                    if (!channel.config().setOption((ChannelOption<Object>) e.getKey(), e.getValue()))  	{
                        logger.warn("Unknown channel option: " + e);
                    }
                } catch (Throwable t) {
                    logger.warn("Failed to set a channel option: " + channel, t);
                }
            }
        }

        final Map<AttributeKey<?>, Object> attrs = attrs();
        synchronized (attrs) {
            for (Entry<AttributeKey<?>, Object> e: attrs.entrySet()) {
                channel.attr((AttributeKey<Object>) e.getKey()).set(e.getValue());
            }
        }
    }
 
 分析tag4.1.2.1，里面将这个TelnetClientInitializer Handler加入到pipeline中，此时handler链是HeadHandler,TelnetClientInitializer,TailHandler,共计3个。
 
 此时程序返回到tag4.1.3继续执行，
 
 	@Override
        public final void AbstractChannel.register(final ChannelPromise promise) {
            if (eventLoop.inEventLoop()) {
                register0(promise);
            } else {
                try {
                    eventLoop.execute(new Runnable() {
                        @Override
                        public void run() {
                            register0(promise);//tag4.1.3.1
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
    

   

         private void AbstractChannel.AbstractUnsafe.register0(ChannelPromise promise) {
            try {
                // check if the channel is still open as it could be closed in the mean time when the register
                // call was outside of the eventLoop
                if (!ensureOpen(promise)) {
                    return;
                }
                doRegister();//tag4.1.3.1.1   
                registered = true;
                promise.setSuccess();
                pipeline.fireChannelRegistered();//tag4.1.3.1.2
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

tag4.1.3.1.1 代码如下
        
         @Override
    protected void AbstractNioChannel.doRegister() throws Exception {
        boolean selected = false;
        for (;;) {
            try {
                selectionKey = javaChannel().register(eventLoop().selector, 0, this);//tag4.1.3.1.2.1
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
    
tag4.1.3.1.2.1 把selector注册到javaChannel上；然后程序继续执行tag4.1.3.1.2代码。

	@Override
    public ChannelPipeline fireChannelRegistered() {
        head.fireChannelRegistered();
        return this;
    }

	@Override
    public ChannelHandlerContext fireChannelRegistered() {
        DefaultChannelHandlerContext next = findContextInbound(MASK_CHANNEL_REGISTERED);
        next.invoker.invokeChannelRegistered(next);
        return this;
    }
    
    @Override
    @SuppressWarnings("unchecked")
    public final void ChannelInitializer.channelRegistered(ChannelHandlerContext ctx) throws Exception {
        ChannelPipeline pipeline = ctx.pipeline();
        boolean success = false;
        try {
            initChannel((C) ctx.channel());//tag4.1.3.1.2.1
            pipeline.remove(this);//tag4.1.3.1.2.2
            ctx.fireChannelRegistered();//tag4.1.3.1.2.3
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
后面的逻辑和服务端类似，此时执行的handler是TelnetClientInitializer，并执行ChannelInitializer的channelRegistered方法，channelRegistered方法里面接着调用了initChannel。

标记 tag4.1.3.1.2.1 代码如下 

 	@Override
    public void TelnetClientInitializer.initChannel(SocketChannel ch) throws Exception {
        ChannelPipeline pipeline = ch.pipeline();

        // Add the text line codec combination first,
        pipeline.addLast("framer", new DelimiterBasedFrameDecoder(
                8192, Delimiters.lineDelimiter()));
        pipeline.addLast("decoder", DECODER);
        pipeline.addLast("encoder", ENCODER);

        // and then business logic.
        pipeline.addLast("handler", CLIENTHANDLER);
    }
    
在完成tag4.1.3.1.2.2的` pipeline.remove(this);`后，此时handler链如下：HeadHandler，DelimiterBasedFrameDecoder，StringDecoder，StringEncoder，TelnetClientHandler， TailHandler。

接着程序又开始执行下一个handler，最终找到TailHandler的channelRegistered方法。TailHandler的channelRegistered方法是空方法。

此时 tag4.1 的代码执行结束，开始执行 tag4.2的代码

private static void doConnect0(
            final ChannelFuture regFuture, final Channel channel,
            final SocketAddress remoteAddress, final SocketAddress localAddress, final ChannelPromise promise) {

        // This method is invoked before channelRegistered() is triggered.  Give user handlers a chance to set up
        // the pipeline in its channelRegistered() implementation.
        channel.eventLoop().execute(new Runnable() {
            @Override
            public void run() {
                if (regFuture.isSuccess()) {
                    if (localAddress == null) {
                        channel.connect(remoteAddress, promise);//tag4.2.1
                    } else {
                        channel.connect(remoteAddress, localAddress, promise);
                    }
                    promise.addListener(ChannelFutureListener.CLOSE_ON_FAILURE);
                } else {
                    promise.setFailure(regFuture.cause());
                }
            }
        });
    }

	@Override
    public ChannelFuture AbstractChannel.connect(SocketAddress remoteAddress, ChannelPromise promise) {
        return pipeline.connect(remoteAddress, promise);
    }
    
经过一番计算，找到HeadHandler，执行unsafe的方法。
    
    @Override
        public void HeadHandler.connect(
                ChannelHandlerContext ctx,
                SocketAddress remoteAddress, SocketAddress localAddress,
                ChannelPromise promise) throws Exception {
            unsafe.connect(remoteAddress, localAddress, promise);
        }
     
 AbstractNioChannel.AbstractNioUnsafe的 connect方法如下：
    
      @Override
        public void connect(
                final SocketAddress remoteAddress, final SocketAddress localAddress, final ChannelPromise promise) {
            if (!ensureOpen(promise)) {
                return;
            }

            try {
                if (connectPromise != null) {
                    throw new IllegalStateException("connection attempt already made");
                }

                boolean wasActive = isActive();
                if (doConnect(remoteAddress, localAddress)) {//tag4.2.1.1
                    fulfillConnectPromise(promise, wasActive);//tag4.2.1.2
                } else {
                    connectPromise = promise;
                    requestedRemoteAddress = remoteAddress;

                    // Schedule connect timeout.
                    int connectTimeoutMillis = config().getConnectTimeoutMillis();
                    if (connectTimeoutMillis > 0) {
                        connectTimeoutFuture = eventLoop().schedule(new Runnable() {
                            @Override
                            public void run() {//tag4.2.1.3
                                ChannelPromise connectPromise = AbstractNioChannel.this.connectPromise;
                                ConnectTimeoutException cause =
                                        new ConnectTimeoutException("connection timed out: " + remoteAddress);
                                if (connectPromise != null && connectPromise.tryFailure(cause)) {
                                    close(voidPromise());
                                }
                            }
                        }, connectTimeoutMillis, TimeUnit.MILLISECONDS);
                    }

                    promise.addListener(new ChannelFutureListener() {
                        @Override
                        public void operationComplete(ChannelFuture future) throws Exception {
                            if (future.isCancelled()) {
                                if (connectTimeoutFuture != null) {
                                    connectTimeoutFuture.cancel(false);
                                }
                                connectPromise = null;
                                close(voidPromise());
                            }
                        }
                    });
                }
            } catch (Throwable t) {
                if (t instanceof ConnectException) {
                    Throwable newT = new ConnectException(t.getMessage() + ": " + remoteAddress);
                    newT.setStackTrace(t.getStackTrace());
                    t = newT;
                }
                promise.tryFailure(t);
                closeIfClosed();
            }
        }
        
tag4.2.1.1 代码如下，进行了bind本地端口和connect远程服务器的操作。 

  	@Override
    protected boolean doConnect(SocketAddress remoteAddress, SocketAddress localAddress) throws Exception {
        if (localAddress != null) {
            javaChannel().socket().bind(localAddress);
        }

        boolean success = false;
        try {
            boolean connected = javaChannel().connect(remoteAddress);//tag4.2.1.1.1
            if (!connected) {
                selectionKey().interestOps(SelectionKey.OP_CONNECT);//tag4.2.1.1.2
            }
            success = true;
            return connected;
        } finally {
            if (!success) {
                doClose();
            }
        }
    }       
    
  
tag4.2.1.1.2 里面执行了connect远程服务器的操作，，我的机器上该方法返回false（返回值详见connect方法说明）。然后会触发执行selectionKey().interestOps(SelectionKey.OP_CONNECT);

需要额外说明的是，此时服务器会触发channelActivi事件。在本例的服务端代码里，会在客户端连接时，发送消息给客户端。不过先暂时忽略服务端和客户端的数据交互，下文分析。

然后tag4.2.1.1 执行结束，由于此时的返回值是false，所以不会执行tag4.2.1.2的 `fulfillConnectPromise(promise, wasActive);`
 
然后程序继续执行tag4.2.1.3 代码，进行连接超时处理：如果设置了超时时间，那么等待指定的超时时间后，再看看是否已经连接上。如果连不上，则设置失败状态。

接着开始下一个事件循环，由于在tag4.2.1.1.2执行了`selectionKey().interestOps(SelectionKey.OP_CONNECT)`操作，会进入到下面的代码。这里我们重点关注tag4.3的代码。
 
 	private static void NioEventLoop.processSelectedKey(SelectionKey k, AbstractNioChannel ch) {
     	//略XXXX
                  if ((readyOps & SelectionKey.OP_CONNECT) != 0) {
                // remove OP_CONNECT as otherwise Selector.select(..) will always return without blocking
                // See https://github.com/netty/netty/issues/924
                int ops = k.interestOps();
                ops &= ~SelectionKey.OP_CONNECT;
                k.interestOps(ops);

                unsafe.finishConnect();//tag4.3
        
        //略XXXX

    }
    
    @Override
        public void finishConnect() {
            // Note this method is invoked by the event loop only if the connection attempt was
            // neither cancelled nor timed out.

            assert eventLoop().inEventLoop();
            assert connectPromise != null;

            try {
                boolean wasActive = isActive();
                doFinishConnect();//tag4.3.1
                fulfillConnectPromise(connectPromise, wasActive);//tag4.3.2
            } catch (Throwable t) {
                if (t instanceof ConnectException) {
                    Throwable newT = new ConnectException(t.getMessage() + ": " + requestedRemoteAddress);
                    newT.setStackTrace(t.getStackTrace());
                    t = newT;
                }

                // Use tryFailure() instead of setFailure() to avoid the race against cancel().
                connectPromise.tryFailure(t);
                closeIfClosed();
            } finally {
                // Check for null as the connectTimeoutFuture is only created if a connectTimeoutMillis > 0 is used
                // See https://github.com/netty/netty/issues/1770
                if (connectTimeoutFuture != null) {
                    connectTimeoutFuture.cancel(false);
                }
                connectPromise = null;
            }
        }
 
	    @Override
    protected void NioSocketChannel.doFinishConnect() throws Exception {
        if (!javaChannel().finishConnect()) {
            throw new Error();
        }
    }
    
在执行完下面的`boolean promiseSet = promise.trySuccess(); `方法后，实例代码中的` Channel ch = b.connect(host, port).sync().channel();`就执行完毕了，然后主线程就阻塞在实例代码中的` String line = in.readLine();`这句代码里了。
    
    private void AbstractNioChannel.AbstractNioUnsafe.fulfillConnectPromise(ChannelPromise promise, boolean wasActive) {
            // trySuccess() will return false if a user cancelled the connection attempt.
            boolean promiseSet = promise.trySuccess();

            // Regardless if the connection attempt was cancelled, channelActive() event should be triggered,
            // because what happened is what happened.
            if (!wasActive && isActive()) {
                pipeline().fireChannelActive();//tag4.3.2.1
            }

            // If a user cancelled the connection attempt, close the channel, which is followed by channelInactive().
            if (!promiseSet) {
                close(voidPromise());
            }
        }
        
     
    @Override
    public ChannelPipeline fireChannelActive() {
        head.fireChannelActive();//tag4.3.2.1.1

        if (channel.config().isAutoRead()) {
            channel.read();//tag4.3.2.1.2
        }

        return this;
    }    
    
 此时，继续执行tag4.3.2.1的代码，进而执行tag4.3.2.1.1的代码，最终执行TailHandler.channelActive方法。由于TailHandler类内部的方法基本都是空实现，所以不再贴代码了。然后再执行tag4.3.2.1.2的`channel.read();`代码，最终执行了AbstractNioChannel.doBeginRead()方法的`selectionKey.interestOps(interestOps | readInterestOp);`，等同于执行了`selectionKey.interestOps(SelectionKey.OP_READ);`。

此时方法返回，在NioEventLoop.run()经过了一些简单的数据清理后，然后有机会对服务端的channelActive时发送的数据进行处理了(在tag4.2.1.1.2曾经提过)。客户端和服务端交互过程详见下篇。
 
---
{% include JB/setup %}
