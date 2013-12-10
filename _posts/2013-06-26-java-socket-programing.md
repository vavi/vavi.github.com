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

TCP通讯需要操作系统支持
传统socket采用per connection per thread 来提升性能
在NIO,由于网络传递数据的速度远远低于cpu的处理速度.并且传递的数据通常放到tcp的协议栈缓存中. 操作系统知道哪些socket的数据包已经准备好了,这样周期性的调用操作系统的Selector.select方法,获取数据准备好的连接.然后针对连接,再从中获取SelectionKey的读写等intOps,进行数据处理.

阻塞,异步  见知乎 ,主要区别是阻塞,非阻塞都是同步的.阻塞通常是因为外部原因(比如等待io或者网络,线程锁 等原因).

NIO优于IO的是因为: 底层OS使用面向块的,可以重复读取.

NIO SOCKET(单个线程管理所有SOCKET的连接,当该SOCKET的数据准备好后,它再通知应用程序进行处理) 优于传统SOCKET(PER CONNECTION PER THREAD) 的原因是因为 在高并发下,每个线程占用1m内存(由-xss参数指定内存大小).**占用内存较大**.另外,是由于socket 在read inputstream 和 write ouputstream时容易因为网络慢而阻塞,导致系统来回切换线程上下文(**切换上下文开销较大**)

现在突然想到,其实为什么java io API的read 方法是将输入流的数据写入到其参数数组内.假如说作为返回值,读入流是一个无穷无尽的数据,则方法可能一直不能得到返回,这样容易导致系统得不到及时响应.


连接线程和处理线程分离开

流 vs 缓冲区 
对数据流使用缓冲,能够极大地提升网络程序的性能.

 
请注意DatagramChannel和SocketChannel实现定义读和写功能的接口而ServerSocketChannel不实现。ServerSocketChannel负责监听传入的连接和创建新的SocketChannel对象，它本身从不传输数据。在我们具体讨论每一种socket通道前，您应该了解socket和socket通道之间的关系。之前的章节中有写道，通道是一个连接I/O服务导管并提供与该服务交互的方法。就某个socket而言，它不会再次实现与之对应的socket通道类中的socket协议API，而java.net中已经存在的socket通道都可以被大多数协议操作重复使用。全部socket通道类（DatagramChannel、SocketChannel和ServerSocketChannel）在被实例化时都会创建一个对等socket对象。这些是我们所熟悉的来自java.net的类（Socket、ServerSocket和DatagramSocket），它们已经被更新以识别通道。对等socket可以通过调用socket( )方法从一个通道上获取。此外，这三个java.net类现在都有getChannel( )方法。虽然每个socket通道（在java.nio.channels包中）都有一个关联的java.net socket对象，却并非所有的socket都有一个关联的通道。如果您用传统方式（直接实例化）创建了一个Socket对象，它就不会有关联的SocketChannel并且它的getChannel( )方法将总是返回null。Socket通道委派协议操作给对等socket对象。如果在通道类中存在似乎重复的socket方法，那么将有某个新的或者不同的行为同通道类上的这个方法相关联。3.5.1 非阻塞模式Socket通道可以在非阻塞模式下运行。这个陈述虽然简单却有着深远的含义。传统Java socket的阻塞性质曾经是Java程序可伸缩性的最重要制约之一。非阻塞I/O是许多复杂的、高性能的程序构建的基础。要把一个socket通道置于非阻塞模式，我们要依靠所有socket通道类的公有超级类：SelectableChannel。下面的方法就是关于通道的阻塞模式的： public abstract class SelectableChannel extends AbstractChannel implements Channel { // This is a partial API listing public abstract void configureBlocking (boolean block) throws IOException; public abstract


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

操作系统的一项最重要的功能就是处理I/O请求并通知各个线程它们的数据已经准备好了。选择器类提供了这种抽象，使得Java代码能够以可移植的方式，请求底层的操作系统提供就绪选择服务。

大多数网络服务流程 
Read requestDecode requestProcess serviceEncode replySend reply
服务器要监听,绑定端口.

Scalable IO in Java(NIO 职责分配糟糕?)

register( )方法位于SelectableChannel类，尽管通道实际上是被注册到选择器上的.
一个单独的通道对象可以被注册到多个选择器上。可以调用isRegistered( )方法来检查一个通道是否被注册到任何一个选择器上。这个方法没有提供关于通道被注册到哪个选择器上的信息，而只能知道它至少被注册到了一个选择器上。此外，在一个键被取消之后，直到通道被注销为止，可能有时间上的延迟。这个方法只是一个提示，而不是确切的答案。任何一个通道和选择器的注册关系都被封装在一个SelectionKey对象中。keyFor( )方法将返回与该通道和指定的选择器相关的键。如果通道被注册到指定的选择器上，那么相关的键将被返回。如果它们之间没有注册关系，那么将返回null。
一个键表示了一个特定的通道对象和一个特定的选择器对象之间的注册关系。您可以看到前两个方法中反映了这种关系。channel( )方法返回与该键相关的SelectableChannel对象，而selector( )则返回相关的Selector对象
当应该终结这种关系的时候，可以调用SelectionKey对象的cancel( )方法。可以通过调用isValid( )方法来检查它是否仍然表示一种有效的关系。当键被取消时，它将被放在相关的选择器的已取消的键的集合里。注册不会立即被取消，但键会立即失效（参见4.3节）。当再次调用select( )方法时（或者一个正在进行的select()调用结束时），已取消的键的集合中的被取消的键将被清理掉，并且相应的注销也将完成。通道会被注销，而新的SelectionKey将被返回。当通道关闭时，所有相关的键会自动取消（记住，一个通道可以被注册到多个选择器上）。当选择器关闭时，所有被注册到该选择器的通道都将被注销，并且相关的键将立即被无效化（取消）。一旦键被无效化，调用它的与选择相关的方法就将抛出CancelledKeyException。
一个SelectionKey对象包含两个以整数形式进行编码的比特掩码：一个用于指示那些通道/选择器组合体所关心的操作(instrest集合)，另一个表示通道准备好要执行的操作（ready集合）。当前的interest集合可以通过调用键对象的interestOps( )方法来获取。最初，这应该是通道被注册时传进来的值。这个interset集合永远不会被选择器改变，但您可以通过调用interestOps( )方法并传入一个新的比特掩码参数来改变它。interest集合也可以通过将通道注册到选择器上来改变（实际上使用一种迂回的方式调用interestOps( )），就像4.1.2小节中描的那样。当相关的Selector上的select( )操作正在进行时改变键的interest集合，不会影响那个正在进行的选择操作。所有更改将会在select( )的下一个调用中体现出来。
可以通过调用键的readyOps( )方法来获取相关的通道的已经就绪的操作。ready集合是interest集合的子集，并且表示了interest集合中从上次调用select( )以来已经就绪的那些操作。例如，下面的代码测试了与键关联的通道是否就绪。如果就绪，就将数据读取出来，写入一个缓冲区，并将它送到一个consumer（消费者）方法中。if ((key.readyOps( ) & SelectionKey.OP_READ) != 0) {134myBuffer.clear( ); key.channel( ).read (myBuffer); doSomethingWithBuffer (myBuffer.flip( )); }
if (key.isWritable( )) 等价于： if ((key.readyOps( ) & SelectionKey.OP_WRITE) != 0)
key.attach (myObject); 和 jQuery的 api 风格类似.
Selector类的核心是选择过程。这个名词您已经在之前看过多次了——现在应该解释一下了。基本上来说，选择器是对select( )、poll( )等本地调用(native call)或者类似的操作系统特定的系统调用的一个包装。但是Selector所作的不仅仅是简单地向本地代码传送参数。它对每个选择操作应用了特定的过程。对这个过程的理解是合理地管理键和它们所表示的状态信息的基础。选择操作是当三种形式的select( )中的任意一种被调用时，由选择器执行的。不管是哪一种形式的调用，下面步骤将被执行：1.已取消的键的集合将会被检查。如果它是非空的，每个已取消的键的集合中的键将从另外两个集合中移除，并且相关的通道将被注销。这个步骤结束后，已取消的键的集合将是空的。
2.已注册的键的集合中的键的interest集合将被检查。在这个步骤中的检查执行过后，对interest集合的改动不会影响剩余的检查过程。一旦就绪条件被定下来，底层操作系统将会进行查询，以确定每个通道所关心的操作的真实就绪状态。依赖于特定的select( )方法调用，如果没有通道已经准备好，线程可能会在这时阻塞，通常会有一个超时值。直到系统调用完成为止，这个过程可能会使得调用线程睡眠一段时间，然后当前每个通道的就绪状态将确定下来。对于那些还没准备好的通道将不会执行任何的操作。对于那些操作系统指示至少已经准备好interest集合中的一种操作的通道，将执行以下两种操作中的一种： a.如果通道的键还没有处于已选择的键的集合中，那么键的ready集合将被清空，然后表示操作系统发现的当前通道已经准备好的操作的比特掩码将被设置。b.否则，也就是键在已选择的键的集合中。键的ready集合将被表示操作系统发现的当前已经准备好的操作的比特掩码更新。所有之前的已经不再是就绪状态的操作不会被清除。事实上，所有的比特位都不会被清理。由操作系统决定的ready集合是与之前的ready集合按位分离的，一旦键被放置于选择器的已选择的键的集合中，它的ready集合将是累积的。比特位只会被设置，不会被清理。3.步骤2可能会花费很长时间，特别是所激发的线程处于休眠状态时。与该选择器相关的键可能会同时被取消。当步骤2结束时，步骤1将重新执行，以完成任意一个在选择进行的过程中，键已经被取消的通道的注销。4.select操作返回的值是ready集合在步骤2中被修改的键的数量，而不是已选择的键的集合中的通道的总数。返回值不是已准备好的通道的总数，而是从上一个select( )调用之后进入就绪状态的通道的数量。之前的调用中就绪的，并且在本次调用中仍然就绪的通道不会被计入，而那些在前一次调用中已经就绪但已经不再处于就绪状态的通道也不会被计入。这些通道可能仍然在已选择的键的集合中，但不会被计入返回值中。返回值可能是0。使用内部的已取消的键的集合来延迟注销，是一种防止线程在取消键时阻塞，并防止与正在进行的选择操作冲突的优化。注销通道是一个潜在的代价很高的操作，这可能需要重新分配资源（请记住，键是与通道相关的，并且可能与它们相关的通道对象之间有复杂的交互）。清理已取消的键，并在选择操作之前和之后立即注销通道，可以消除它们可能正好在选择的过程中执行的潜在棘手问题。这是另一个兼顾健壮性的折中方案。
调用wakeup( ) 调用Selector对象的wakeup( )方法将使得选择器上的第一个还没有返回的选择操作立即返回。如果当前没有在进行中的选择，那么下一次对select( )方法的一种形式的调用将立即返回。后续的选择操作将正常进行。在选择操作之间多次调用wakeup( )方法与调用它一次没有什么不同。有时这种延迟的唤醒行为并不是您想要的。您可能只想唤醒一个睡眠中的线程，而使得后续的选择继续正常地进行。您可以通过在调用wakeup( )方法后调用selectNow( )方法来绕过这个问题。尽管如此，如果您将您的代码构造为合理地关注于返回值和执行选择集合，那么即使下一个select( )方法的调用在没有通道就绪时就立即返回，也应该不会有什么不同。不管怎么说，您应该为可能发生的事件做好准备。

## NIO SOCKET 示例

三个新套接字通道，即 ServerSocketChannel、SocketChannel 和 DatagramChannel;Selector 这4个类均使用open方法来创建实例.

Channel 
Buffer
Selector
SelectionKey

与 Observable 模式比较 

需要将之前创建的一个或多个可选择的通道注册到选择器对象中。一个表示通道和选择器的键将会被返回。选择键会记住您关心的通道。它们也会追踪对应的通道是否已经就绪。当您调用一个选择器对象的select( )方法时，相关的键建会被更新，用来检查所有被注册到该选择器的通道。您可以获取一个键的集合，从而找到当时已经就绪的通道。通过遍历这些键，您可以选择出每个从上次您调用select( )开始直到现在，已经就绪的通道。


## 常见参数设置 

*  setTcpNoDelay(boolean on)  ,true则禁用Nagle's algorithm,false则使用Nagle's algorithm.Nagle's algorithm是指:小的包(1字节)在发送前会组合成大点的包.在发送另一个包之前,本地主机需要等待远程主机对前一个包的回应.该算法问题是,如果远程系统没有尽可能快地将回应包发送会本地,那么依赖小数据量信息稳定传递的英用程序会变得很慢.因此,在传输小包时(如游戏,及时通讯时),建议将参数设置为true,便于将包迅速发到远程主机.

# 参考
<<JAVA网络编程>>
提高 Linux 上 socket 性能 http://www.ibm.com/developerworks/cn/linux/l-hisock.html
Scalable IO in Java
{% include JB/setup %}
