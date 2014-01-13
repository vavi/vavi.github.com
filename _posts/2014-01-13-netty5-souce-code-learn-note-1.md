---

layout: post
title: "Netty5 客户端启动过程源码分析"
description: ""
category: 
tags: []

---
## 说明

客户端和服务端比较相似，所以本篇会会一定程度上略去重复的部分。

io.netty.example.telnet.TelnetClient


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
    
下面开始第二段代码分析
    
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
    
    分析tag4.1代码
    
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
 分析tag4.1.1.1.1，里面调用DefaultChannelPipeline构造器，和服务端的逻辑应用，故不作分析。
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
                    if (!channel.config().setOption((ChannelOption<Object>) e.getKey(), e.getValue())) {
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
        
tag4.2.1.1 代码如下，里面执行了`boolean connected = javaChannel().connect(remoteAddress);`，我的机器上该方法返回false（返回值详见connect方法说明）。
 
  	@Override
    protected boolean doConnect(SocketAddress remoteAddress, SocketAddress localAddress) throws Exception {
        if (localAddress != null) {
            javaChannel().socket().bind(localAddress);//tag4.2.1.1.1
        }

        boolean success = false;
        try {
            boolean connected = javaChannel().connect(remoteAddress);
            if (!connected) {
                selectionKey().interestOps(SelectionKey.OP_CONNECT);
            }
            success = true;
            return connected;
        } finally {
            if (!success) {
                doClose();
            }
        }
    }       
    
 tag4.2.1.3 ，如果设置了超时时间，那么等待指定的超时时间后，再看看是否已经连接上。如果连不上，则设置失败状态。
 
## todo

 
  	@Override
        public void AbstractNioChannel.AbstractNioUnsafe.finishConnect() {
            // Note this method is invoked by the event loop only if the connection attempt was
            // neither cancelled nor timed out.

            assert eventLoop().inEventLoop();
            assert connectPromise != null;

            try {
                boolean wasActive = isActive();
                doFinishConnect();
                fulfillConnectPromise(connectPromise, wasActive);
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

 
     
## 客户端发送数据

tag4.2.1.1.1 这里执行了`javaChannel().socket().bind(localAddress);`，会导致服务端程序接收到数据包并作出响应。

	private static void NioEventLoop.processSelectedKey(SelectionKey k, AbstractNioChannel ch) {
        final NioUnsafe unsafe = ch.unsafe();
        if (!k.isValid()) {
            // close the channel if the key is not valid anymore
            unsafe.close(unsafe.voidPromise());
            return;
        }

        try {
            int readyOps = k.readyOps();
            // Also check for readOps of 0 to workaround possible JDK bug which may otherwise lead
            // to a spin loop
            if ((readyOps & (SelectionKey.OP_READ | SelectionKey.OP_ACCEPT)) != 0 || readyOps == 0) {
                unsafe.read();//tag1
                if (!ch.isOpen()) {
                    // Connection already closed - no need to handle write.
                    return;
                }
            }
            if ((readyOps & SelectionKey.OP_WRITE) != 0) {
                // Call forceFlush which will also take care of clear the OP_WRITE once there is nothing left to write
                ch.unsafe().forceFlush();
            }
            if ((readyOps & SelectionKey.OP_CONNECT) != 0) {
                // remove OP_CONNECT as otherwise Selector.select(..) will always return without blocking
                // See https://github.com/netty/netty/issues/924
                int ops = k.interestOps();
                ops &= ~SelectionKey.OP_CONNECT;
                k.interestOps(ops);

                unsafe.finishConnect();//tag2
            }
        } catch (CancelledKeyException e) {
            unsafe.close(unsafe.voidPromise());
        }
    }

此时会执行tag1 的 `unsafe.read();`方法。

	@Override
        public void read() {
            assert eventLoop().inEventLoop();
            if (!config().isAutoRead()) {
                removeReadOp();
            }

            final ChannelConfig config = config();
            final int maxMessagesPerRead = config.getMaxMessagesPerRead();
            final boolean autoRead = config.isAutoRead();
            final ChannelPipeline pipeline = pipeline();
            boolean closed = false;
            Throwable exception = null;
            try {
                for (;;) {
                    int localRead = doReadMessages(readBuf);//tag1.1
                    if (localRead == 0) {
                        break;
                    }
                    if (localRead < 0) {
                        closed = true;
                        break;
                    }

                    if (readBuf.size() >= maxMessagesPerRead | !autoRead) {
                        break;
                    }
                }
            } catch (Throwable t) {
                exception = t;
            }

            int size = readBuf.size();
            for (int i = 0; i < size; i ++) {
                pipeline.fireChannelRead(readBuf.get(i));//tag1.2
            }
            readBuf.clear();
            pipeline.fireChannelReadComplete();//tag1.3

            if (exception != null) {
                if (exception instanceof IOException) {
                    // ServerChannel should not be closed even on IOException because it can often continue
                    // accepting incoming connections. (e.g. too many open files)
                    closed = !(AbstractNioMessageChannel.this instanceof ServerChannel);
                }

                pipeline.fireExceptionCaught(exception);
            }

            if (closed) {
                if (isOpen()) {
                    close(voidPromise());
                }
            }
        }


	 @Override
    protected int NioServerSocketChannel.doReadMessages(List<Object> buf) throws Exception {
        SocketChannel ch = javaChannel().accept();//tag1.1.1

        try {
            if (ch != null) {
             buf.add(new NioSocketChannel(this, childEventLoopGroup().next(), ch));//tag1.1.2
                return 1;
            }
        } catch (Throwable t) {
            logger.warn("Failed to create a new channel from an accepted socket.", t);

            try {
                ch.close();
            } catch (Throwable t2) {
                logger.warn("Failed to close a socket.", t2);
            }
        }

        return 0;
    }
  
 在tag1.1.1中执行accept方法，该方法在JDK doc中简单描述如下：如果该channel出于非阻塞状态而且没有等待(pending)的连接，那么该方法会返回null；否则该方法会阻塞直到连接可用或者发生I/O错误。
 
 然后，此时实际上Client发送了connect请求并且服务端是处于non-blocking模式下，那么这个`accept()`会返回一个不为null的channel。
 
 然后继续执行tag1.1.2代码，并使用了不同的EventLoop实例，即childEventLoopGroup().next()。
 
 然后`doReadMessages`返回1，并执行`readBuf.size() >= maxMessagesPerRead | !autoRead`,`readBuf.size() >= maxMessagesPerRead`值为false；`!autoRead`仍为false，则|操作后仍为false

此时继续执行外面的for循环。由于不满足“如果该channel出于非阻塞状态而且没有等待(pending)的连接，那么该方法会返回null”这个约束，所以此时`doReadMessages`返回0；并最终退出循环。
  
  
此时程序执行tag1.2代码，readBuf变量的值为服务端accept后的 NioSocketChannel 。再回忆下，此时的handler链是HeadHandler，ServerBootstrapAcceptor和TailHandler。

	@Override
    public ChannelPipeline fireChannelRead(Object msg) {
        head.fireChannelRead(msg);
        return this;
    }
 
 由于`ServerBootstrap`覆写了 channelRead 方法,所以程序执行了`ServerBootstrapAcceptor.channelRead`方法。
 
 		@Override
        @SuppressWarnings("unchecked")
        public void channelRead(ChannelHandlerContext ctx, Object msg) {
            Channel child = (Channel) msg;

            child.pipeline().addLast(childHandler);//tag1.2.1

            for (Entry<ChannelOption<?>, Object> e: childOptions) {
                try {
                    if (!child.config().setOption((ChannelOption<Object>) e.getKey(), e.getValue())) {
                        logger.warn("Unknown channel option: " + e);
                    }
                } catch (Throwable t) {
                    logger.warn("Failed to set a channel option: " + child, t);
                }
            }

            for (Entry<AttributeKey<?>, Object> e: childAttrs) {
                child.attr((AttributeKey<Object>) e.getKey()).set(e.getValue());
            }

            child.unsafe().register(child.newPromise());//tag1.2.2
        }

在执行tag1.2.1代码段前，child的handler链是HeadHandler，TailHandler，请注意不要和parent的Handler混淆。在执行完tag1.2.1后，此时的handler链是HeadHandler，TelnetServerInitializer和TailHandler 

然后开始执行 tag1.2.2 `child.unsafe().register(child.newPromise());`

	 @Override
        public final void register(final ChannelPromise promise) {
            if (eventLoop.inEventLoop()) {
                register0(promise);
            } else {
                try {
                    eventLoop.execute(new Runnable() {
                        @Override
                        public void run() {
                            register0(promise);//tag1.2.2.1
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

此时会在另一个线程里面执行tag1.2.2.1  register0(promise);

 private void AbstractChannel.register0(ChannelPromise promise) {
            try {
                // check if the channel is still open as it could be closed in the mean time when the register
                // call was outside of the eventLoop
                if (!ensureOpen(promise)) {
                    return;
                }
                doRegister();//tag1.2.2.1.1
                registered = true;
                promise.setSuccess();
                pipeline.fireChannelRegistered();//tag1.2.2.1.2
                if (isActive()) {
                    pipeline.fireChannelActive();//tag1.2.2.1.3
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
   
  tag1.2.2.1.1，前文已介绍
        
	 @Override
    protected void doRegister() throws Exception {
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
    
   tag1.2.2.1.2，当执行fireChannelRegistered时，里面会继续执行TelnetServerInitializer父类的channelRegistered方法。
       
   	@Override
    public ChannelPipeline fireChannelRegistered() {
        head.fireChannelRegistered();
        return this;
    } 
 
    @Override
    @SuppressWarnings("unchecked")
    public final void channelRegistered(ChannelHandlerContext ctx) throws Exception 	{
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
    
  
  在channelRegistered方法中，调用TelnetServerInitializer.initChannel方法，进而完成将下面的几个handler加入到Handler链中。此时，child handler链是HeadHandler，DelimiterBasedFrameDecoder，StringDecoder，StringEncoder，TelnetServerHandler和TailHandler。
  
    @Override
    public void TelnetServerInitializer.initChannel(SocketChannel ch) throws Exception {
        ChannelPipeline pipeline = ch.pipeline();

        // Add the text line codec combination first,
        pipeline.addLast("framer", new DelimiterBasedFrameDecoder(
                8192, Delimiters.lineDelimiter()));
        // the encoder and decoder are static as these are sharable
        pipeline.addLast("decoder", DECODER);
        pipeline.addLast("encoder", ENCODER);

        // and then business logic.
        pipeline.addLast("handler", SERVERHANDLER);
    }
    
  此时完成tag1.2.2.1.2 代码段的执行，然后继续执行 tag1.2.2.1.3 的代码段 `pipeline.fireChannelActive();`，
	@Override
    public ChannelPipeline DefaultChannelPipeline.fireChannelActive() {
        head.fireChannelActive();

        if (channel.config().isAutoRead()) {
            channel.read();
        }

        return this;
    }
  
  由于仅TelnetServerHandler覆写了channelActive方法，所以仅执行了TelnetServerHandler

  
   @Override
    public void TelnetServerHandler.channelActive(ChannelHandlerContext ctx) throws Exception {
        // Send greeting for a new connection.
        ctx.write(
                "Welcome to " + InetAddress.getLocalHost().getHostName() + "!\r\n");//tag1.2.2.1.3.1
        ctx.write("It is " + new Date() + " now.\r\n");
        ctx.flush();//tag1.2.2.1.3.2
    }
    
	@Override
    public ChannelFuture DefaultChannelHandlerContext.write(Object msg) {
        return write(msg, newPromise());
    } 
    
     @Override
    public ChannelFuture DefaultChannelHandlerContext.write(Object msg, ChannelPromise promise) {
        DefaultChannelHandlerContext next = findContextOutbound(MASK_WRITE);
        next.invoker.invokeWrite(next, msg, promise);
        return promise;
    }
    
     @Override
    public void DefaultChannelHandlerInvoker.invokeWrite(ChannelHandlerContext ctx, Object msg, ChannelPromise promise) {
        if (msg == null) {
            throw new NullPointerException("msg");
        }

        validatePromise(ctx, promise, true);

        if (executor.inEventLoop()) {
            invokeWriteNow(ctx, msg, promise);
        } else {
            AbstractChannel channel = (AbstractChannel) ctx.channel();
            int size = channel.estimatorHandle().size(msg);
            if (size > 0) {
                ChannelOutboundBuffer buffer = channel.unsafe().outboundBuffer();
                // Check for null as it may be set to null if the channel is closed already
                if (buffer != null) {
                    buffer.incrementPendingOutboundBytes(size);
                }
            }
            safeExecuteOutbound(WriteTask.newInstance(ctx, msg, size, promise), promise, msg);
        }
    } 
    
执行了StringEncoder父类的write方法。由于笔者目前对这部分细节不感兴趣，所以暂时略去分析(TODO)。   
在StringEncoder快执行完成了，又通过执行` ctx.write(out.get(sizeMinusOne), promise);` 来继续执行下一个handler：HeadHandler。

	 @Override
        public void HeadHandler.write(ChannelHandlerContext ctx, Object msg, ChannelPromise promise) throws Exception {
            unsafe.write(msg, promise);
        }   
    
	@Override
        public void AbstractChannel.AbstractUnsafe.write(Object msg, ChannelPromise promise) {
            if (!isActive()) {
                // Mark the write request as failure if the channel is inactive.
                if (isOpen()) {
                    promise.tryFailure(NOT_YET_CONNECTED_EXCEPTION);
                } else {
                    promise.tryFailure(CLOSED_CHANNEL_EXCEPTION);
                }
                // release message now to prevent resource-leak
                ReferenceCountUtil.release(msg);
            } else {
                outboundBuffer.addMessage(msg, promise);//暂时略去不分析 TODO
            }
        }    
该 outboundBuffer.addMessage(msg, promise) 将msg存储到ChannelOutboundBuffer中。至此，简单分析了ctx.write()方法，下面接着执行tag1.2.2.1.3.2 `ctx.flush();`方法

        
    @Override
    public ChannelHandlerContext flush() {
        DefaultChannelHandlerContext next = findContextOutbound(MASK_FLUSH);
        next.invoker.invokeFlush(next);
        return this;
    } 
    
     @Override
        public void HeadHandler.flush(ChannelHandlerContext ctx) throws Exception {
            unsafe.flush();
        }   
        
        
	@Override
        public void AbstractNioChannel.flush() {
            ChannelOutboundBuffer outboundBuffer = this.outboundBuffer;
            if (outboundBuffer == null) {
                return;
            }

            outboundBuffer.addFlush();
            flush0();
        }


最终，调用了NioSocketChannel.doWrite方法，在其内部执行了`final long localWrittenBytes = ch.write(nioBuffers, 0, nioBufferCnt);`这句话，从而保证数据写入到socket缓冲区中。
        
 	@Override
    protected void NioSocketChannel.doWrite(ChannelOutboundBuffer in) throws Exception {
        for (;;) {
            // Do non-gathering write for a single buffer case.
            final int msgCount = in.size();
            if (msgCount <= 1) {
                super.doWrite(in);
                return;
            }

            // Ensure the pending writes are made of ByteBufs only.
            ByteBuffer[] nioBuffers = in.nioBuffers();
            if (nioBuffers == null) {
                super.doWrite(in);
                return;
            }

            int nioBufferCnt = in.nioBufferCount();
            long expectedWrittenBytes = in.nioBufferSize();

            final SocketChannel ch = javaChannel();
            long writtenBytes = 0;
            boolean done = false;
            boolean setOpWrite = false;
            for (int i = config().getWriteSpinCount() - 1; i >= 0; i --) {
                final long localWrittenBytes = ch.write(nioBuffers, 0, nioBufferCnt);
                if (localWrittenBytes == 0) {
                    setOpWrite = true;
                    break;
                }
                expectedWrittenBytes -= localWrittenBytes;
                writtenBytes += localWrittenBytes;
                if (expectedWrittenBytes == 0) {
                    done = true;
                    break;
                }
            }

            if (done) {
                // Release all buffers
                for (int i = msgCount; i > 0; i --) {
                    in.remove();
                }

                // Finish the write loop if no new messages were flushed by in.remove().
                if (in.isEmpty()) {
                    clearOpWrite();
                    break;
                }
            } else {
                // Did not write all buffers completely.
                // Release the fully written buffers and update the indexes of the partially written buffer.

                for (int i = msgCount; i > 0; i --) {
                    final ByteBuf buf = (ByteBuf) in.current();
                    final int readerIndex = buf.readerIndex();
                    final int readableBytes = buf.writerIndex() - readerIndex;

                    if (readableBytes < writtenBytes) {
                        in.progress(readableBytes);
                        in.remove();
                        writtenBytes -= readableBytes;
                    } else if (readableBytes > writtenBytes) {
                        buf.readerIndex(readerIndex + (int) writtenBytes);
                        in.progress(writtenBytes);
                        break;
                    } else { // readableBytes == writtenBytes
                        in.progress(readableBytes);
                        in.remove();
                        break;
                    }
                }

                incompleteWrite(setOpWrite);
                break;
            }
        }
    }        
    
 至此，程序返回到tag1.3 继续执行`pipeline.fireChannelReadComplete();`   
    
channel.config() 何时初始化？

channel方法委托给pipiline，根据in（head）还是outbound（tail），开始委托给tail还是head，然后再进一步委托给unsafe

太多异步，怎么测试？
bind 和 active 状态测试 

NioEventLoop.wakeup(boolean inEventLoop) 避免两次唤醒


---
{% include JB/setup %}
