---
layout: post
title: "Java网络编程"
description: ""
category: 
tags: [java socket]
---
# 前言
人类需要交流,所以才会有语言,文字这些东西. 交流的双方必须使用彼此理解的语言或者文字才能沟通.

在计算机网络世界中,两个计算机需要通信的话,也必须使用计算机之间能够理解的协议.在选择协议的时候,要区分传输协议(TCP,UDP)和数据协议(JSON,XML等).

理解计算机网络编程,需要了解TCP/IP协议族.TCP/IP协议族共分为四层,链路层,网络层,传输层和应用层. 链路层主要和网卡驱动打交道,网络层主要指IP协议,传输层指TCP/UDP.应用层则很多,比如HTTP,FTP协议等等.

现代操作系统的实现一般都包括了TCP/IP协议的实现库.所以在编写计算机网络通信程序时,一般调用操作系统提供的API即可.需要注意的是,链路层,网络层,传输层在内核态工作,而应用层则在用户态工作.

下图是[@淘叔度](http://weibo.com/1804559491/A9VajwP07)大牛画的socket函数调用与TCP状态迁移的时序图,很清楚.


![]({{ BASE_PATH }}/images/java-socket/os-api-and-tcp-state-transfer.jpg )	

# 正文
## 常见SOCKET API
### InetAddress
必要时,使用DNS

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
Socket 类使用native 方法,通过操作系统的本地TCP栈完成通信.

一般使用Socket的步骤如下:

* 使用构造器创建socket对象,并可以设置相关连接参数
* 与远程主机建立连接
* 获得socket的输入,输出流,进行读写
* 断开连接

建立连接还是不建立连接 

* 不建立连接的构造方法: 
  *  Socket()
  *  Socket(Proxy proxy)
  *	 Socket(SocketImpl impl),可见性为protected,用于子类继承
* 建立连接的构造方法
  *  其他构造方法(JDK1.7中,构造参数中都包含远程主机的IP,端口)
* 之所以提供不建立连接的构造函数,是因为有时需要在建立连接前,设置一些连接参数	

指定通讯时的本地端口:Socket(InetAddress address, int port, InetAddress localAddr, int localPort)  

Socket.getPort(),Socket.getInetAddress() 方法名get后面加个Remote该多好

Socket.isConnected() 表示该socket是否成功连接过远程注解,而不是表示是否正在连接远程主机 
connect
bind
accept

SOCKET API并没有对外公布底层的listen 方法 

backLog

## NIO SOCKET
NIO优于IO的是因为: 底层OS使用面向块的,可以重复读取.

NIO SOCKET(单个线程管理所有SOCKET的连接,当该SOCKET的数据准备好后,它再通知应用程序进行处理) 优于传统SOCKET(PER CONNECTION PER THREAD) 的原因是因为 在高并发下,每个线程占用1m内存(由-xss参数指定内存大小).**占用内存较大**.另外,是由于socket 在read inputstream 和 write ouputstream时容易因为网络慢而阻塞,导致系统来回切换线程上下文(**切换上下文开销较大**)

现在突然想到,其实为什么java io API的read 方法是将输入流的数据写入到其参数数组内.假如说作为返回值,读入流是一个无穷无尽的数据,则方法可能一直不能得到返回,这样容易导致系统得不到及时响应.


连接线程和处理线程分离开

流 vs 缓冲区 
对数据流使用缓冲,能够极大地提升网络程序的性能.

在NIO中,缓冲区已经成为NIO的基础部分,读写数据流时,必须通过缓冲区(OIO中可以直接读写流). 在native 实现中,可以将缓冲区直接与硬件或内存联系起来,或者使用其他非常高效的实现.

从编程角度来看,流基于字节,通道基于块.流的基本概念都是一次传送一个字节数据,通道是传送缓冲区的数据块.

Java 网络编程 404 页说明:"为什么JAVA NIO的方法没有遵守 getter/setter 的命名方式?我无法解释.这点遭受到JCP的极力谴责"

如果您选择在ServerSocket上调用accept( )方法，那么它会同任何其他的ServerSocket表现一样的行为：总是阻塞并返回一个java.net.Socket对象。如果您选择在ServerSocketChannel上调用accept( )方法则会返回SocketChannel类型的对象，返回的对象能够在非阻塞模式下运行。

如果以非阻塞模式被调用，当没有传入连接在等待时，ServerSocketChannel.accept( )会立即返回null。正是这种检查连接而不阻塞的能力实现了可伸缩性并降低了复杂性。可选择性也因此得到实现.

每个SocketChannel对象创建时都是同一个对等的java.net.Socket对象串联的。静态的open( )方法可以创建一个新的SocketChannel对象，而在新创建的SocketChannel上调用socket( )方法能返回它对等的Socket对象；在该Socket上调用getChannel( )方法则能返回最初的那个SocketChannel。虽然每个SocketChannel对象都会创建一个对等的Socket对象，反过来却不成立。直接创建的Socket对象不会关联SocketChannel对象，它们的getChannel( )方法只返回null。
 
 `sc.connect (addr); while ( ! sc.finishConnect( )) { doSomethingUseful( ); }`
 
 Unix系统中，管道被用来连接一个进程的输出和另一个进程的输入。java.nio.channels包内的Pipe类实现一个管道范例，不过它所创建的管道是进程内（在Java虚拟机进程内部）而非进程间使用的。Pipe类创建一对提供环回机制的Channel对象。这两个通道的远端是连接起来的，以便任何写在SinkChannel对象上的数据都能出现在SourceChannel对象上.
 
 此时，您可能在想管道到底有什么作用。您不能使用Pipe在操作系统级的进程间建立一个类Unix管道（您可以使用SocketChannel来建立）。Pipe的source通道和sink通道提供类似java.io.PipedInputStream和java.io.PipedOutputStream所提供的功能，不过它们可以执行全部的通道语义。请注意，SinkChannel和SourceChannel都由AbstractSelectableChannel引申而来（所以也是从SelectableChannel引申而来），这意味着pipe通道可以同选择器一起使用（参见第四章）。
 
 管道可以被用来仅在同一个Java虚拟机内部传输数据。虽然有更加有效率的方式来在线程之间传输数据，但是使用管道的好处在于封装性。生产者线程和用户线程都能被写道通用的Channel API中。根据给定的通道类型，相同的代码可以被用来写数据到一个文件、socket或管道。选择器可以被用来检查管道上的数据可用性，如同在socket通道上使用那样地简单。这样就可以允许单个用户线程使用一个Selector来从多个通道有效地收集数据，并可任意结合网络连接或本地工作线程使用。因此，这些对于可伸缩性、冗余度以及可复用性来说无疑都是意义重大的。



## NIO SOCKET 示例

三个新套接字通道，即 ServerSocketChannel、SocketChannel 和 DatagramChannel;Selector 这4个类均使用open方法来创建实例.

## 常见参数设置 

*  setTcpNoDelay(boolean on)  ,true则禁用Nagle's algorithm,false则使用Nagle's algorithm.Nagle's algorithm是指:小的包(1字节)在发送前会组合成大点的包.在发送另一个包之前,本地主机需要等待远程主机对前一个包的回应.该算法问题是,如果远程系统没有尽可能快地将回应包发送会本地,那么依赖小数据量信息稳定传递的英用程序会变得很慢.因此,在传输小包时(如游戏,及时通讯时),建议将参数设置为true,便于将包迅速发到远程主机.

# 参考
<<JAVA网络编程>>
提高 Linux 上 socket 性能 http://www.ibm.com/developerworks/cn/linux/l-hisock.html

{% include JB/setup %}
