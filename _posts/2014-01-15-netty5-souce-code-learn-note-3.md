---

layout: post
title: "Netty5源码分析--3.客户端与服务端交互过程详解"
description: ""
category: 开源项目
tags: [netty]

---
书接上文，由于服务端增加了`TelnetServerHandler`，而该Handler覆写了channelActive方法，所以在客户端connect服务端时，服务端会向客户端写出数据。而由于客户端增加了`TelnetClientHandler`，而该Handler覆写了messageReceived方法。所以在接收到服务端消息后，会将服务端内容打印出来。

 	@Override
    public void TelnetServerHandler.channelActive(ChannelHandlerContext ctx) throws Exception {
        // Send greeting for a new connection.
        ctx.write(
                "Welcome to " + InetAddress.getLocalHost().getHostName() + "!\r\n");
        ctx.write("It is " + new Date() + " now.\r\n");
        ctx.flush();
    }
    
    @Override
    protected void TelnetClientHandler.messageReceived(ChannelHandlerContext ctx, String msg) throws Exception {
        System.err.println(msg);
    }

## 服务端发送数据


当客户端执行了`javaChannel().connect(remoteAddress);`方法后，会导致服务端程序接收到数据包，并作出响应。

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

                unsafe.finishConnect();
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

在tag1.1.1中，执行accept方法，该方法在JDK doc中简单描述如下：如果该channel处于非阻塞状态而且没有等待(pending)的连接，那么该方法会返回null；否则该方法会阻塞直到连接可用或者发生I/O错误。此时实际上Client发送了connect请求并且服务端是处于non-blocking模式下，那么这个`accept()`会返回一个不为null的channel。

 然后继续执行tag1.1.2代码，并使用了不同的EventLoop实例，即childEventLoopGroup().next()。 接着`doReadMessages`返回1。然后程序继续执行上面的代码：`readBuf.size() >= maxMessagesPerRead | !autoRead`,`readBuf.size() >= maxMessagesPerRead`值为false；`!autoRead`仍为false，则|操作后仍为false。此时继续执行外面的for循环，由于不满足“如果该channel出于非阻塞状态而且没有等待(pending)的连接，那么该方法会返回null”这个约束，所以在第二次执行`doReadMessages`返回0，并最终退出循环。


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
  
 
  
下面程序继续执行tag1.2代码，readBuf变量的值为服务端accept后的 NioSocketChannel，readBuf.size()值为1。不过先回忆下，此时的handler链是HeadHandler，ServerBootstrapAcceptor和TailHandler。

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

在执行tag1.2.1代码段前，child的handler链是HeadHandler，TailHandler。请读者注意，不要和parent的Handler链混淆。在执行完tag1.2.1后，此时的handler链是HeadHandler，TelnetServerInitializer和TailHandler 

然后开始执行 tag1.2.2 `child.unsafe().register(child.newPromise());`。有一个小细节就是，此时会开一个新worker线程，去执行这个register0操作。 

	 @Override
        public final void AbstractChannel.AbstractUnsafe.register(final ChannelPromise promise) {
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

在tag1.2.2.1，开始执行 register0(promise);

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
   
  tag1.2.2.1.1，主要执行`javaChannel().register(eventLoop().selector, 0, this);`；有点奇怪的是，这里的ops参数是0。TODO。
        
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
        head.fireChannelActive();//tag1.2.2.1.3.1

        if (channel.config().isAutoRead()) {
            channel.read();//tag1.2.2.1.3.2
        }

        return this;
    }
  
  由于child handler链里只有TelnetServerHandler覆写了channelActive方法，所以仅执行了TelnetServerHandler。

  
   @Override
    public void TelnetServerHandler.channelActive(ChannelHandlerContext ctx) throws Exception {
        // Send greeting for a new connection.
        ctx.write(
                "Welcome to " + InetAddress.getLocalHost().getHostName() + "!\r\n");//tag1.2.2.1.3.1.1
        ctx.write("It is " + new Date() + " now.\r\n");
        ctx.flush();//tag1.2.2.1.3.1.2

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
            invokeWriteNow(ctx, msg, promise);//tag1.2.2.1.3.1.1.1
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
   
  在tag1.2.2.1.3.1.1.1处，执行了StringEncoder父类的MessageToMessageEncoder.write方法。由于笔者目前对这部分细节不感兴趣，所以暂时略去分析(TODO)。 
    
在StringEncoder的父类MessageToMessageEncoder的write方法的finally块里，通过执行` ctx.write(out.get(sizeMinusOne), promise);` 来继续执行下一个handler：HeadHandler，从而完成 ctx.write()方法的Handler执行。
   
  (ChannelHandlerContext ctx, Object msg, ChannelPromise promise) throws Exception
 
    

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
该 outboundBuffer.addMessage(msg, promise) 将msg存储到ChannelOutboundBuffer中。至此，简单分析了ctx.write()方法，下面接着执行tag1.2.2.1.3.1.2 `ctx.flush();`方法

        
    @Override
    public ChannelHandlerContext DefaultChannelHandlerContext.flush() {
        DefaultChannelHandlerContext next = findContextOutbound(MASK_FLUSH);
        next.invoker.invokeFlush(next);
        return this;
    } 
    
     @Override
        public void HeadHandler.flush(ChannelHandlerContext ctx) throws Exception {
            unsafe.flush();
        }   
        
        
	@Override
        public void AbstractChannel.AbstractUnsafe.flush() {
            ChannelOutboundBuffer outboundBuffer = this.outboundBuffer;
            if (outboundBuffer == null) {
                return;
            }

            outboundBuffer.addFlush();
            flush0();
        }

	 	@Override
        protected void AbstractNioChannel.AbstractNioUnsafe.flush0() {
            // Flush immediately only when there's no pending flush.
            // If there's a pending flush operation, event loop will call forceFlush() later,
            // and thus there's no need to call it now.
            if (isFlushPending()) {
                return;
            }
            super.flush0();
        }


	    protected void AbstractChannel.AbstractUnsafe.flush0() {
            if (inFlush0) {
                // Avoid re-entrance
                return;
            }

            final ChannelOutboundBuffer outboundBuffer = this.outboundBuffer;
            if (outboundBuffer == null || outboundBuffer.isEmpty()) {
                return;
            }

            inFlush0 = true;

            // Mark all pending write requests as failure if the channel is inactive.
            if (!isActive()) {
                try {
                    if (isOpen()) {
                        outboundBuffer.failFlushed(NOT_YET_CONNECTED_EXCEPTION);
                    } else {
                        outboundBuffer.failFlushed(CLOSED_CHANNEL_EXCEPTION);
                    }
                } finally {
                    inFlush0 = false;
                }
                return;
            }

            try {
                doWrite(outboundBuffer);//tag1.2.2.1.3.1.2.1
            } catch (Throwable t) {
                outboundBuffer.failFlushed(t);
            } finally {
                inFlush0 = false;
            }
        }

在tag1.2.2.1.3.1.2.1处，最终调用了NioSocketChannel.doWrite方法，在该方法内部执行了`final long localWrittenBytes = ch.write(nioBuffers, 0, nioBufferCnt);`这句话，从而保证数据写入到socket缓冲区中。
        
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
                final long localWrittenBytes = ch.write(nioBuffers, 0, nioBufferCnt);//数据最终写出
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
                    clearOpWrite();//tag1.2.2.1.3.1.2.1.1
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
    
  就在刚执行了`final long localWrittenBytes = ch.write(nioBuffers, 0, nioBufferCnt);`这句话后，客户端立即进入了NioEventLoop.processSelectedKey()方法中，准备开始读入数据了。不过此刻稍缓下，先把服务端的流程走完。
  
  还有一个非常重要的小细节，就是在tag1.2.2.1.3.1.2.1.1处，执行了AbstractNioByteChannel.clearOpWrite() 方法，避免发生CPU100%问题。
  
    
由于在执行tag1.2.2 时，那时是新开了一个线程执行的。所以，当新线程执行时，旧线程继续执行tag1.3，即`pipeline.fireChannelReadComplete();` ，最终该线程执行TailHandler.channelReadComplete()，该方法也是空实现。

## 客户端接收数据
    
就在服务端刚刚执行完 javaChannel.write()方法后，客户端就收到服务端的数据，开始执行`NioEventLoop.processSelectedKey()`方法。在其内部执行`unsafe.read();`方法。


   	if ((readyOps & (SelectionKey.OP_READ | SelectionKey.OP_ACCEPT)) != 0 || readyOps == 0) {
                unsafe.read();//tag2
                if (!ch.isOpen()) {
                    // Connection already closed - no need to handle write.
                    return;
                }
   }

此时，程序开始执行tag2 方法 

AbstractNioByteChannel.NioByteUnsafe.read()

		@Override
        public void AbstractNioByteChannel.NioByteUnsafe.read() {
            final ChannelConfig config = config();
            final ChannelPipeline pipeline = pipeline();
            final ByteBufAllocator allocator = config.getAllocator();
            final int maxMessagesPerRead = config.getMaxMessagesPerRead();
            RecvByteBufAllocator.Handle allocHandle = this.allocHandle;
            if (allocHandle == null) {
                this.allocHandle = allocHandle = config.getRecvByteBufAllocator().newHandle();
            }
            if (!config.isAutoRead()) {
                removeReadOp();
            }

            ByteBuf byteBuf = null;
            int messages = 0;
            boolean close = false;
            try {
                int byteBufCapacity = allocHandle.guess();
                int totalReadAmount = 0;
                do {
                    byteBuf = allocator.ioBuffer(byteBufCapacity);
                    int writable = byteBuf.writableBytes();
                    int localReadAmount = doReadBytes(byteBuf);//tag2.1
                    if (localReadAmount <= 0) {
                        // not was read release the buffer
                        byteBuf.release();
                        close = localReadAmount < 0;
                        break;
                    }

                    pipeline.fireChannelRead(byteBuf);//tag2.2
                    byteBuf = null;

                    if (totalReadAmount >= Integer.MAX_VALUE - localReadAmount) {
                        // Avoid overflow.
                        totalReadAmount = Integer.MAX_VALUE;
                        break;
                    }

                    totalReadAmount += localReadAmount;
                    if (localReadAmount < writable) {
                        // Read less than what the buffer can hold,
                        // which might mean we drained the recv buffer completely.
                        break;
                    }
                } while (++ messages < maxMessagesPerRead);

                pipeline.fireChannelReadComplete();//tag2.3
                allocHandle.record(totalReadAmount);//tag2.4

                if (close) {
                    closeOnRead(pipeline);
                    close = false;
                }
            } catch (Throwable t) {
                handleReadException(pipeline, byteBuf, t, close);
            }
        }
    }

在tag2.1代码中，最终执行`UnpooledUnsafeDirectByteBuf.setBytes`方法，在该方法内部`in.read(tmpBuf);`，从而完成网络数据的读取。

	 @Override
    public int UnpooledUnsafeDirectByteBuf.setBytes(int index, ScatteringByteChannel in, int length) throws IOException {
        ensureAccessible();
        ByteBuffer tmpBuf = internalNioBuffer();
        tmpBuf.clear().position(index).limit(index + length);
        try {
            return in.read(tmpBuf);
        } catch (ClosedChannelException e) {
            return -1;
        }
    }
    
 接着执行tag2.2的代码，执行`pipeline.fireChannelRead(byteBuf);`,开始执行`DelimiterBasedFrameDecoder(ByteToMessageDecoder).channelRead()` 方法。该类可以通过指定分隔符，把ByteBuf再分成多条消息。所以，在执行完` callDecode(ctx, cumulation, out);`方法后，变量out还有两条记录，也就是服务端发过来的两条消息。
 

 
 @Override
    public void channelRead(ChannelHandlerContext ctx, Object msg) throws Exception {
        if (msg instanceof ByteBuf) {
            RecyclableArrayList out = RecyclableArrayList.newInstance();
            try {
                ByteBuf data = (ByteBuf) msg;
                first = cumulation == null;
                if (first) {
                    cumulation = data;
                } else {
                    if (cumulation.writerIndex() > cumulation.maxCapacity() - data.readableBytes()) {
                        expandCumulation(ctx, data.readableBytes());
                    }
                    cumulation.writeBytes(data);
                    data.release();
                }
                callDecode(ctx, cumulation, out);//tag2.2.1
            } catch (DecoderException e) {
                throw e;
            } catch (Throwable t) {
                throw new DecoderException(t);
            } finally {
                if (cumulation != null && !cumulation.isReadable()) {
                    cumulation.release();
                    cumulation = null;
                }
                int size = out.size();
                decodeWasNull = size == 0;

                for (int i = 0; i < size; i ++) {
                    ctx.fireChannelRead(out.get(i));//tag2.2.2
                }
                out.recycle();
            }
        } else {
            ctx.fireChannelRead(msg);
        }
    }
   
接着继续执行tag2.2的代码，从而执行`StringDecoder(MessageToMessageDecoder<I>).channelRead()`方法，在该方法内部，即tag2.2.2.1会执行`StringDecoder.decode(hannelHandlerContext ctx, ByteBuf msg, List<Object> out)`方法，从而完成将字节转成字符串的功能。

	@Override
    public void MessageToMessageDecoder.channelRead(ChannelHandlerContext ctx, Object msg) throws Exception {
        RecyclableArrayList out = RecyclableArrayList.newInstance();
        try {
            if (acceptInboundMessage(msg)) {
                @SuppressWarnings("unchecked")
                I cast = (I) msg;
                try {
                    decode(ctx, cast, out);//tag2.2.2.1
                } finally {
                    ReferenceCountUtil.release(cast);
                }
            } else {
                out.add(msg);
            }
        } catch (DecoderException e) {
            throw e;
        } catch (Exception e) {
            throw new DecoderException(e);
        } finally {
            int size = out.size();
            for (int i = 0; i < size; i ++) {
                ctx.fireChannelRead(out.get(i));//tag2.2.2.2
            }
            out.recycle();
        }
    }
    
在tag2.2.2.2处，会继续执行`TelnetClientHandler(SimpleChannelInboundHandler<I>).channelRead(ChannelHandlerContext, Object)	`方法，在该方法内部会执行TelnetClientHandler.messageReceived方法，在该方法内部执行` System.err.println(msg);`方法。

	@Override
    protected void messageReceived(ChannelHandlerContext ctx, String msg) throws Exception {
        System.err.println(msg);
    }
 
然后代码返回到tag2.2.2处，继续执行下一个循环，从而最终完成两次消息打印。
    
接着代码继续返回到tag2.3处，继续执行`pipeline.fireChannelReadComplete(); `，从而触发了ByteToMessageDecoder和TailHandler的channelReadComplete()方法执行。      

行文到此，服务端和客户端交互分析完毕。后续再进行总结下阅读Netty代码过程的思考


{% include JB/setup %}
