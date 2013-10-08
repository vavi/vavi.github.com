---
layout: post
title: "Java IO 编程"
description: ""
category: 
tags: [J2SE]
---
io 可以分为 disk io,network io 甚至 memory io.

本文主要是disk io.

## OIO(Old IO)

inputstream/outputstream读写字节,reader/writer读写字符;

同样的字节在不同的编码方式下,可以在编辑器中显示为不同的字符

reader/writer 其实是对inputstream/outputstream的封装.

由于disk io 的读写数据的速度远远低于 cpu 处理数据的速度,所以通常在读写时,上面封装一层 buffered stream/reader/writer.

之所以会出现乱码,大部分时因为互相转换字节,字符时,没有指定正确的编码(通常是使用了默认得编码,比如String.getBytes默认使用JVM的默认编码,J2EE容器如tomcat使用了server.xml的ISO-8859-1的编码).


## NIO(New IO)
nio 的flip方法通常是读取流后,将数据写到buffer中.然后在需要将buffer的数据读出来,供应用程序使用.这个时候就需要先调用flip方法,将limit=position,position=0,mark=-1.

nio的clear方法并没有清除掉原来的buffer中的数据,但是由于其position=0,limit=capacity,mark=-1,这样在读取数据时也不会读取到旧的数据

0 <= mark <=position <=limit<=capacity
mark 提供标记,类似书签功能,方便重读.

所有的缓冲区都具有四个属性来提供关于其所包含的数据元素的信息。它们是： 容量（Capacity） 缓冲区能够容纳的数据元素的最大数量。这一容量在缓冲区创建时被设定，并且永远不能被改变。上界（Limit） 缓冲区的第一个不能被读或写的元素。或者说，缓冲区中现存元素的计数。位置（Position） 下一个要被读或写的元素的索引。**位置会自动由相应的get( )和put( )函数更新**。标记（Mark） 一个备忘位置。调用mark( )来设定mark = postion。调用reset( )设定position = mark。标记在设定前是未定义的(undefined)。

我们想把这个缓冲区传递给一个通道，以使内容能被全部写出。**但如果通道现在在缓冲区上执行get()，那么它将从我们刚刚插入的有用数据之外取出未定义数据**。**如果我们将位置值重新设为0，通道就会从正确位置开始获取，但是它是怎样知道何时到达我们所插入数据末端的呢？这就是上界属性被引入的目的。上界属性指明了缓冲区有效内容的末端。**我们需要将上界属性设置为当前位置，然后将位置重置为0。我们可以人工用下面的代码实现： **buffer.limit(buffer.position()).position(0);** 但这种从填充到释放状态的缓冲区翻转是API设计者预先设计好的，他们为我们提供了一个非常便利的函数： **Buffer.flip()**;

Rewind()函数与flip()相似，但不影响上界属性。它只是将位置值设回0。您可以使用rewind()后退，重读已经被翻转的缓冲区中的数据。

compact 函数将当前position和limit之间的元素填充到数组buffer的0到limit-position这个范围内,剩余的buff数组内容不变.limit值修改为和capacity一样大.

slice()创建一个从原始缓冲区的当前位置开始的新缓冲区，并且其容量是原始缓冲区的剩余元素数量（limit-position）。这个新缓冲区与原始缓冲区共享一段数据元素子序列。分割出来的缓冲区也会继承只读和直接属性。

arrayOffset()


从磁盘等设备读出channel的数据,然后写到buffer 中 (channel.read)
http://www.zavakid.com/

为什么buffered会快? nio 面向流,面向块的差别有多大?

传统 Java 平台上的 I/O 抽象工作良好，适应用途广泛。但是当移动大量数据时，这些 I/O 类可伸缩性不强，也没有提供当今大多数操作系统普遍具备的常用 I/O 功能，如文件锁定、非块 I/O、就绪性选择和内存映射

还有一种特殊类型的缓冲区，用于内存映射文件。

Channels 提供了将流和通道之间的互相转换API.

下面节选自<<Java NIO>>,总结得很到位.

然而，在大多数情况下，Java 应用程序并非真的受着 I/O 的束缚。操作系统并非不能快速传送数据，让 Java 有事可做；相反，是 JVM 自身在 I/O 方面效率欠佳。操作系统与 Java 基于流的 I/O模型有些不匹配。操作系统要移动的是大块数据（缓冲区），这往往是在硬件直接存储器存取（DMA）的协助下完成的。而 JVM 的 I/O 类喜欢操作小块数据——单个字节、几行文本。结果，操作系统送来整缓冲区的数据，java.io 的流数据类再花大量时间把它们拆成小块，往往拷贝一个小块就要往返于几层对象。操作系统喜欢整卡车地运来数据，java.io 类则喜欢一铲子一铲子地加工数据。有了 NIO，就可以轻松地把一卡车数据备份到您能直接使用的地方（ByteBuffer 对象）。




当进程请求 I/O 操作的时候，它执行一个系统调用（有时称为陷阱）将控制权移交给内核。C/C++程序员所熟知的底层函数 open( )、read( )、write( )和 close( )要做的无非就是建立和执行适当的**系统调用**。当内核以这种方式被调用，它随即采取任何必要步骤，找到进程所需数据，并把数据传送到用户空间内的指定缓冲区。内核试图对数据进行高速缓存或**预读取**，因此进程所需数据可能已经在内核空间里了。如果是这样，该数据只需简单地拷贝出来即可。如果数据不在内核空间，则进程被挂起，内核着手把数据读进内存。














##Channel

* FileChannel
* ServerSocketChannel
* SocketChannel
* DatagramChannel
 不能直接创建 FileChannel 对象,后3种 可以通过调用相应里地类方法open进行创建Channel对象
 
 FileChannel对象却只能通过在一个打开的RandomAccessFile、FileInputStream或FileOutputStream对象上调用getChannel( )方法来获取
 
 java.net的socket类也有新的getChannel( )方法。这些方法虽然能返回一个相应的socket通道对象，但它们却并非新通道的来源，RandomAccessFile.getChannel( )方法才是。只有在已经有通道存在的时候，它们才返回与一个socket关联的通道；它们永远不会创建新通道



非阻塞模式的通道永远不会让调用的线程休眠。请求的操作要么立即完成，要么返回一个结果表明未进行任何操作。只有面向流的（stream-oriented）的通道，如sockets和pipes才能使用非阻塞模式。将非阻塞I/O和选择器组合起来可以使您的程序利用多路复用I/O（multiplexed I/O）。

Scatter/Gather是一个简单却强大的概念，它是指在多个缓冲区上实现一个简单的I/O操作。对于一个write操作而言，数据是从几个缓冲区按顺序抽取（称为gather）并沿着通道发送的。缓冲区本身并不需要具备这种gather的能力（通常它们也没有此能力）。该gather过程的效果就好比全部缓冲区的内容被连结起来，并在发送数据前存放到一个大的缓冲区中。对于read操作而言，从通道读取的数据会按顺序被散布（称为scatter）到多个缓冲区，将每个缓冲区填满直至通道中的数据或者缓冲区的最大空间被消耗完。大多数现代操作系统都支持本地矢量I/O（native vectored I/O）。当您在一个通道上请求一个Scatter/Gather操作时，该请求会被翻译为适当的本地调用来直接填充或抽取缓冲区。这是一个很大的进步，因为减少或避免了缓冲区拷贝和系统调用。Scatter/Gather应该使用直接的ByteBuffers以从本地I/O获取最大性能优势。

使用得当的话，Scatter/Gather会是一个极其强大的工具。它允许您委托操作系统来完成辛苦活：将读取到的数据分开存放到多个存储桶（bucket）或者将不同的数据区块合并成一个整体。这是一个巨大的成就，因为操作系统已经被高度优化来完成此类工作了。它节省了您来回移动数据的工作，也就避免了缓冲区拷贝和减少了您需要编写、调试的代码数量。既然您基本上通过提供数据容器引用来组合数据，那么按照不同的组合构建多个缓冲区阵列引用，各种数据区块就可以以不同的方式来组合了。

文件通道总是阻塞式的，因此不能被置于非阻塞模式。现代操作系统都有复杂的缓存和预取机制，使得本地磁盘I/O操作延迟很少。网络文件系统一般而言延迟会多些，不过却也因该优化而受益。面向流的I/O的非阻塞范例对于面向文件的操作并无多大意义，这是由文件I/O本质上的不同性质造成的。对于文件I/O，最强大之处在于异步I/O（asynchronous I/O），它允许一个进程可以从操作系统请求一个或多个I/O操作而不必等待这些操作的完成。发起请求的进程之后会收到它请求的I/O操作已完成的通知。异步I/O是一种高级性能，当前的很多操作系统都还不具备。以后的NIO增强也会把异步I/O纳入考虑范围。

锁与文件关联，而不是与通道关联。我们使用锁来判优外部进程，而不是判优同一个Java虚拟机上的线程。


请注意DatagramChannel和SocketChannel实现定义读和写功能的接口而ServerSocketChannel不实现。ServerSocketChannel负责监听传入的连接和创建新的SocketChannel对象，它本身从不传输数据。在我们具体讨论每一种socket通道前，您应该了解socket和socket通道之间的关系。之前的章节中有写道，通道是一个连接I/O服务导管并提供与该服务交互的方法。就某个socket而言，它不会再次实现与之对应的socket通道类中的socket协议API，而java.net中已经存在的socket通道都可以被大多数协议操作重复使用。全部socket通道类（DatagramChannel、SocketChannel和ServerSocketChannel）在被实例化时都会创建一个对等socket对象。这些是我们所熟悉的来自java.net的类（Socket、ServerSocket和DatagramSocket），它们已经被更新以识别通道。对等socket可以通过调用socket( )方法从一个通道上获取。此外，这三个java.net类现在都有getChannel( )方法。虽然每个socket通道（在java.nio.channels包中）都有一个关联的java.net socket对象，却并非所有的socket都有一个关联的通道。如果您用传统方式（直接实例化）创建了一个Socket对象，它就不会有关联的SocketChannel并且它的getChannel( )方法将总是返回null。Socket通道委派协议操作给对等socket对象。如果在通道类中存在似乎重复的socket方法，那么将有某个新的或者不同的行为同通道类上的这个方法相关联。



#参考
http://ifeve.com/java-nio-all/
http://blog.csdn.net/historyasamirror/article/details/5778378
http://www.ibm.com/developerworks/cn/java/j-lo-javaio/
<<Java NIO>>,概述部分总结的相当精彩,后面有些章节翻译的比较差.

{% include JB/setup %}