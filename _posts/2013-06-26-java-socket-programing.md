---
layout: post
title: "JAVA学习笔记--3.Network IO"
description: ""
category: JAVA学习笔记
tags: [JAVA学习笔记]
---
## 引论
人类需要交流,所以才会有语言,文字这些东西. 交流的双方必须使用彼此理解的语言或者文字才能沟通.

在计算机网络世界中,两个计算机需要通信的话,也必须使用计算机之间能够理解的协议.在双方通信时,比如要区分**传输协议**(TCP,UDP)和**数据协议**(二进制，文本（JSON,XML等）).

理解计算机网络编程,需要了解TCP/IP协议族.TCP/IP协议族共分为四层,链路层,网络层,传输层和应用层. 链路层主要和网卡驱动打交道,网络层主要指IP协议,传输层指TCP/UDP.应用层则很多,比如HTTP,FTP协议等等.

现代操作系统的实现一般都包括了TCP/IP协议的实现库.所以在编写计算机网络通信程序时,一般调用操作系统提供的API即可.需要注意的是,链路层,网络层,传输层在内核态工作,而应用层则在用户态工作.

===
## TCP的三次握手和四次挥手


### 三次握手
#### 生活示例
举个实际生活的例子，假设A和B在通过QQ进行语音聊天，A和B都需要插上耳机和话筒。

开始聊天时：

A说：能听见吗  (完成发送SYN_SENT) 

B说：能听见，你能听见我吗？ ()

A说：能听见。 

下面开始吧啦吧啦聊天。。。

=== 
#### 状态迁移 

下图是[@淘叔度](http://weibo.com/1804559491/A9VajwP07)大牛画的socket函数调用与TCP状态迁移的时序图,很清楚.


![]({{ BASE_PATH }}/images/java-socket/os-api-and-tcp-state-transfer.jpg )

===
### 四次挥手
#### 生活示例
接上面的生活示例，结束聊天时（这个例子有点不符合现实）：

A说：我不想说话了，但是我可以听你说（关闭话筒）。 

B说：好吧（B此时可以关闭耳机，反正A此时不会说话了）。

B说：我也不想说话了 （关闭话筒） 

A说：好吧（关闭听筒）

===
#### 状态迁移
下图是我自己画得，感觉画得比较粗糙，但是基本上能够说明东西了。

![]({{ BASE_PATH }}/images/java-socket/4_goodbye.png )


说明1：2*MSL（Max Segment Lifetime，最大分段生存期，指一个TCP报文在Internet上的最长生存时间。每个具体的TCP协议实现都必须选择一个确定的MSL值，RFC 1122建议是2分钟，但BSD传统实现采用了30秒，Linux可以cat /proc/sys/net/ipv4/tcp_fin_timeout看到本机的这个值），然后即可回到CLOSED 可用状态了。
	
说明2：CLOSING状态在上图中没有画出来，该状态表示在发出FIN报文后，理应受到对方的ACK报文，但是却收到对方的FIN报文，表示对方也在进行close操作。所以，CLOSING状态表示双方都在进行close操作。

===
## 常见SOCKET API
### InetAddress
必要时,会自动使用DNS

* java.net.InetAddress.getByName(String host)
* java.net.InetAddress.getAllByName(String host) 
* java.net.InetAddress.getLocalHost() 

不使用dns

* java.net.InetAddress.getByAddress(byte[] addr)
* java.net.InetAddress.getByAddress(String host, byte[] addr) 

### InetAddress vs InetSocketAddress

* InetAddress 表示一个主机的IPV4或者IPV6地址
* InetSocketAddress 表示一个InetAddress和Socket通信的端口号(端口号的作用是用来区分通信的不同应用程序)

### Socket

#### 一般使用Socket的步骤如下:

* 使用构造器创建socket对象,并可以设置相关连接参数（new socket and set options）
* 与远程主机建立连接(connect)
* 获得socket的输入,输出流,进行读写(get inputstream and outputstream)
* 断开连接(close)

#### Socket构造方法 

* 不建立连接的构造方法: 
  *  Socket()
  *  Socket(Proxy proxy)
  *	 Socket(SocketImpl impl),可见性为protected,用于子类继承
* 建立连接的构造方法
  *  其他构造方法(JDK1.7中,构造参数中都包含远程主机的IP,端口)
* 之所以提供不建立连接的构造函数,是因为**有时需要在建立连接前,设置一些连接参数**
* 指定通讯时的本地端口:Socket(InetAddress address, int port, InetAddress localAddr, int localPort)  

#### Socket的其他重要方法

* isConnected():表示该socket是否曾经成功连接过远程主机,并不表示是否正在连接远程主机 
* bind(SocketAddress bindpoint):绑定本地ip和端口，绑定本地ip和端口 
* connect(SocketAddress endpoint, int timeout)：连接远程ip和端口，如果在timeout**毫秒**内仍然不能连接到远程主机，则报SocketTimeoutException错误。

### ServerSocket
#### ServerSocket的其他重要方法
* bind(SocketAddress endpoint, int backlog):绑定本地ip地址和端口，并将backLog参数传递给getImpl().listen(backlog)调用。backlog主要是指当ServerSocket还没执行accept时，这个时候的请求会放在os层面的一个队列里，这个队列的大小即为backlog值.如backlog不够大的话，可能会造成客户端接到连接失败的状况
* Socket accept():该方法一直阻塞，直到对方主机和自身建立连接，并返回代表这个连接的Socket对象。

===
## NIO（NonBlocking IO） SOCKET
### 非阻塞的概念
 阻塞,非阻塞都是同步的，阻塞通常是指因为外部原因(比如等待io或者网络,线程锁 等原因)而发生不得不等待的行为.
### 传统网络服务设计模型 
传统socket采用PER CONNECTION PER THREAD 来提升性能。但是很多时候，连接建立起来了，但是可能由于客户端一直没输入数据（比如网络聊天等），导致占用内存浪费（但是由于因为每个线程占用1m内存(由-xss参数指定内存大小）。而且，是由于socket 在read inputstream 和 write ouputstream时容易因为网络慢而阻塞,导致系统来回切换线程上下文(**切换上下文开销较大**)
### NIO SOCKET的本质
在NIO,由于网络传递数据的速度远远低于CPU的处理数据的速度，并且传递的数据通常放到TCP的协议栈缓存中. 操作系统知道哪些socket的数据包已经准备好了,这样周期性的调用操作系统的Selector.select方法,获取数据准备好的连接。然后针对这些准备好数据的连接,再从中获取SelectionKey的socketChannel,进行处理网络中的数据。

从NIO实现原理来看，操作系统用单个线程来管理哪些连接数据准备好了，并把这些连接维护在一个列表里面，然后客户端程序去遍历这个列表，进行处理。

通常在实际应用中，会使用线程池工具来处理网络数据。

=== 
### NIO SOCKET 隐喻  
* Channel 公路，意指网络的一个连接。
* Buffer 公路上的汽车，意指网络的一个数据包。
* BusinessData 汽车里面的货物，意指数据包里面的业务数据。
* Worker 工人，用来装货和卸货。意指业务程序，读取对端传递的数据和写出自己需要发送出去的数据。
* Selector 公路管理系统，在汽车出站后，通知Worker卸货；在进站后，通知Worker装货。意指网络上每个连接的数据包状态，哪些数据可读等。
* SelectionKey 汽车的行驶状态：出站，在途和进站。意指具体的网络连接，从网络连接中可以取到具体的数据。

=== 
### 常见NIO SOCKET API  
0. ServerSocketChannel、SocketChannel、Selector 这3个类均使用open方法来创建实例。
1. SocketChannel实现定义读和写功能的接口，而ServerSocketChannel不实现。ServerSocketChannel负责监听传入的连接和创建新的SocketChannel对象，它本身从不传输数据。
2. 虽然每个Socket通道都有一个关联的Socket对象，却并非所有的Socket都有一个关联的通道。如果您用传统方式（直接实例化）创建了一个Socket对象，它就不会有关联的SocketChannel并且它的getChannel( )方法将总是返回null。 
3. 一个SelectionKey表示了一个特定的Channel对象和一个特定的Selector对象之间的注册关系。SelectionKey.channel( )方法返回与该键相关的SelectableChannel对象，而SelectionKey.selector( )则返回相关的Selector对象。
4. `sc.connect (addr); while ( ! sc.finishConnect( )) { doSomethingUseful( ); }` 这个看完netty代码再补充。
5. if (key.isWritable( )) 等价于： if ((key.readyOps( ) & SelectionKey.OP_WRITE) != 0)

===
## 常见参数设置 

*  setTcpNoDelay(boolean on)：on为true则禁用Nagle's algorithm,false则使用Nagle's algorithm.Nagle's algorithm是指:小的包(1字节)在发送前会组合成大点的包.在发送另一个包之前,本地主机需要等待远程主机对前一个包的回应.该算法问题是,如果远程系统没有尽可能快地将回应包发送会本地,那么依赖小数据量信息稳定传递的英用程序会变得很慢.因此,在传输小包时(如游戏,及时通讯时),建议将参数设置为true,便于将包迅速发到远程主机.
*  setSoLinger(boolean on, int linger)：SO_LINGER选项规定，当socket关闭时如何处理尚未发送的数据。如果on为false或者linger为0，那么当socket关闭时，所有未发送的数据都会被丢弃；否则系统会等待linger秒，等待数据发送和接收回应。当过去相应秒数后，socket就被关闭，所有未发送数据被丢弃，也不再接收任何数据。需要注意的是，**该linger单位是秒，不是毫秒。**
*  setSoTimeout(int timeout):当尝试读取socket数据时，read()方法会阻尽可能多的时间，已得到足够多的数据。设置SO_TIMEOUT后，就可以保证阻塞的时间不会大于timeout毫米数。需要注意的是，**该timeout单位是毫秒，不是秒，与linger不同。**
*  setKeepAlive(boolean on):如果启用SO_KEEPTIME，客户端偶尔通过一个空闲连接发送一个数据包（一般两小时一次，各个平台可以有自己不同的实现），以确保服务器未崩溃。
*  setReuseAddress(boolean on):当on是true：当socket关闭时，且处于TIME_WAIT状态，允许其他连接程序复用本地bind的端口。

===
## 吐槽
1. Socket.getPort(),Socket.getInetAddress() 方法名get后面加个Remote该多好。
2. Java 网络编程 404 页吐槽:"为什么JAVA NIO的方法没有遵守 getter/setter 的命名方式?我无法解释.这点遭受到JCP的极力谴责"
3. Socket类为什么名称不是ClientSocket，与ServerSocket相对应？原因是因为，ServerSocket负责监听传入的连接和创建新的Socket对象，它本身从不传输数据。也就是说，Socket这个类作为服务端ServerSocket.accept() 方法执行结果。

===
## 参考

http://hi.baidu.com/suxinpingtao51/item/be5f71b3a907dbef4ec7fd0e

http://hellojava.info/?p=109，Java程序员也应该知道的一些网络知识

http://www.zhihu.com/question/19732473 ，怎样理解阻塞非阻塞与同步异步的区别？

JAVA网络编程

提高 Linux 上 socket 性能 http://www.ibm.com/developerworks/cn/linux/l-hisock.html

Scalable IO in Java.pdf
{% include JB/setup %}
