---
layout: post
title: "JAVA 学习重点整理笔记--整理中"
description: ""
category: 
tags: []
---
 
## THREAD
### 什么是多线程
多线程程序会额外增加CPU和内存的消耗,以及导致实现复杂.
 
  1. 线程安全性是指"永远不发生糟糕的事情",线程活跃性是指"某件正确的事情最终会发生”.

可重入的意思是线程可以重复获得它已经持有的锁。一般而言,重入锁的实现方法是:线程标识+锁的计数器



Mutex导致上下文切换 ；SpinLock属于busy-waiting类型的锁，也称自旋锁，Spinlock优点：没有昂贵的系统调用，不会导致线程的状态切换(用户态->内核态)，执行速度快


   * Spinlock缺点：一直占用cpu，而且在执行过程中还会锁bus总线，锁总线时其他处理器不能使用总线.
   * Mutex优点：不会忙等，得不到锁会sleep.
   * Mutex缺点：sleep时会陷入到内核态，需要昂贵的系统调用


在上下文切换过程中，CPU会停止处理当前运行的程序，并保存当前程序运行的具体位置以便之后继续运行。
在三种情况下可能会发生上下文切换：中断处理，多任务处理，用户态切换.
线程还需要一些内存来维持它本地的堆栈,也需要占用操作系统中一些资源来管理线程.
上下文切换会带来直接和间接两种因素影响程序性能的消耗. 直接消耗包括: CPU寄存器需要保存和加载, 系统调度器的代码需要执行, TLB实例需要重新加载, CPU 的pipeline需要刷掉; 间接消耗指的是多核的cache之间得共享数据变得无效了。


调用wait方法会产生如下操作:当前线程就进入阻塞状态。释放目标对象的同步锁，但是除此之外的其他锁依然由该线程持有

调用Notify会产生如下操作：随意选择一个线程，线程T必须重新获得目标对象的锁，直到有线程调用wait释放该锁


理解Java内存模型 在java中，所有实例域、静态域和数组元素存储在堆内存中，堆内存在线程之间共享（本文使用“共享变量”这个术语代指实例域，静态域和数组元素）。局部变量（Local variables），方法定义参数（java语言规范称之为formal method parameters）和异常处理器参数（exception handler parameters）不会在线程之间共享，它们不会有内存可见性问题，也不受内存模型的影响。

JMM决定一个线程对共享变量的写入何时对另一个线程可见。从抽象的角度来看，JMM定义了线程和主内存之间的抽象关系：线程之间的共享变量存储在主内存（main memory）中，每个线程都有一个私有的本地内存（local memory），本地内存中存储了该线程以读/写共享变量的副本。本地内存是JMM的一个抽象概念，并不真实存在。它涵盖了缓存，写缓冲区，寄存器以及其他的硬件和编译器优化。Java内存模型的抽象示意图如下：


从上图来看，线程A与线程B之间如要通信的话，必须要经历下面2个步骤：

  1. 首先，线程A把本地内存A中更新过的共享变量刷新到主内存中去。
  2. 然后，线程B到主内存中去读取线程A之前已更新过的共享变量。






重排序分三种类型：

  1. 编译器优化的重排序。编译器在不改变单线程程序语义的前提下，可以重新安排语句的执行顺序。
  2. 指令级并行的重排序。现代处理器采用了指令级并行技术（Instruction-Level Parallelism， ILP）来将多条指令重叠执行。如果不存在数据依赖性，处理器可以改变语句对应机器指令的执行顺序。
  3. 内存系统的重排序。由于处理器使用缓存和读/写缓冲区，这使得加载和存储操作看上去可能是在乱序执行。

对于编译器，JMM的编译器重排序规则会禁止特定类型的编译器重排序（不是所有的编译器重排序都要禁止）。对于处理器重排序，JMM的处理器重排序规则会要求java编译器在生成指令序列时，插入特定类型的内存屏障（memory barriers，intel称之为memory fence）指令，通过内存屏障指令来禁止特定类型的处理器重排序（不是所有的处理器重排序都要禁止）。



现代的处理器使用写缓冲区来临时保存向内存写入的数据。写缓冲区可以保证指令流水线持续运行，它可以避免由于处理器停顿下来等待向内存写入数据而产生的延迟。同时，通过以批处理的方式刷新写缓冲区，以及合并写缓冲区中对同一内存地址的多次写，可以减少对内存总线的占用。虽然写缓冲区有这么多好处，但每个处理器上的写缓冲区，仅仅对它所在的处理器可见。这个特性会对内存操作的执行顺序产生重要的影响：处理器对内存的读/写操作的执行顺序，不一定与内存实际发生的读/写操作顺序一致！



由于写缓冲区仅对自己的处理器可见，它会导致处理器执行内存操作的顺序可能会与内存实际的操作执行顺序不一致。由于现代的处理器都会使用写缓冲区，因此现代的处理器都会允许对写-读操做重排序。


cas底层的cpu指令 CMPXCHG
•一个Atomic RMW操作，若Operand 1为Memory，则CMPXCHG指令还需要Lock指令配合 (Lock prefix)；

能够告诉Compiler，不要进行Reordering，这个机制，就是Compiler Memory Barrier。


 能够告诉Compiler，不要进行Reordering，这个机制，就是Compiler Memory Barrier。

在编译后的汇编中，Compiler Memory Barrier消失，CPU不能感知到Compiler Memory Barrier的存在，这点与后面提到的CPU Memory Barrier有所不同；因此CPU Memory Barrier是一条真正的指令，存在于编译后的汇编代码中；

4 CPU Reordering Types
LoadLoad
读读乱序
LoadStore
读写乱序
StoreLoad
写读乱序
StoreStore
写写乱序
4种基本的CPU Memory Barriers
LoadLoad Barrier
LoadStore Barrier
StoreLoad Barrier
StoreStore Barrier

更为复杂的CPU Memory Barriers
Store Barrier (Read Barrier)
所有在Store Barrier前的Store操作，必须在Store Barrier指令前执行完毕；而所有Store Barrier指令后的Store操作，必须在Store指令执行结束后才能开始；

Store Barrier只针对Store(Write)操作，对Load无任何影响；

Load Barrier (Write Barrier)
将Store Barrier的功能，全部换为针对Load操作即可；

Full Barrier
Load + Store Barrier，Full Barrier两边的任何操作，均不可交换顺序；

Only CPU Memory Barrier
     asm volatile(“mfence”);

CPU + Compiler Memory Barrier
     asm volatile(“mfence” ::: ”memory”);


Question？
除了CPU本身提供的Memory Barrier指令之外，是否有其他的方式实现Memory Barrier？

Yes! We Need Lock Instruction’s Help!
Reads or writes cannot be reordered with I/O instructions, locked instructions, or serializing instructions.

解读
既然read/write不能穿越locked instructions进行reordering，那么所有带有lock prefix的指令，都构成了一个天然的Full  Memory Barrier；


Linux(x86, x86-64)
smp_rmb()
smp_wmb()
smp_mb()
 
Read Acquire + Write Release语义，是所有锁实现的基础(Spinlock, Mutex, RWLock, ...)，所有被[Read Acquire, Write Release]包含的区域，即构成了一个临界区，临界区内的指令，确保不会在临界区外运行。因此，Read Acquire又称为Lock Acquire，Write Release又称为Unlock Release；
 
JMM把内存屏障指令分为下列四类：
屏障类型指令示例说明LoadLoad BarriersLoad1; LoadLoad; Load2确保Load1数据的装载，之前于Load2及所有后续装载指令的装载。StoreStore BarriersStore1; StoreStore; Store2确保Store1数据对其他处理器可见（刷新到内存），之前于Store2及所有后续存储指令的存储。LoadStore BarriersLoad1; LoadStore; Store2确保Load1数据装载，之前于Store2及所有后续的存储指令刷新到内存。StoreLoad BarriersStore1; StoreLoad; Load2确保Store1数据对其他处理器变得可见（指刷新到内存），之前于Load2及所有后续装载指令的装载。StoreLoad Barriers会使该屏障之前的所有内存访问指令（存储和装载指令）完成之后，才执行该屏障之后的内存访问指令。


StoreLoad Barriers是一个“全能型”的屏障，它同时具有其他三个屏障的效果。现代的多处理器大都支持该屏障（其他类型的屏障不一定被所有处理器支持）。执行该屏障开销会很昂贵，因为当前处理器通常要把写缓冲区中的数据全部刷新到内存中（buffer fully flush）。




JSR-133提出了happens-before的概念，通过这个概念来阐述操作之间的内存可见性。如果一个操作执行的结果需要对另一个操作可见，那么这两个操作之间必须存在happens-before关系。这里提到的两个操作既可以是在一个线程之内，也可以是在不同线程之间。 与程序员密切相关的happens-before规则如下：

   * 程序顺序规则：一个线程中的每个操作，happens- before 于该线程中的任意后续操作。
   * 监视器锁规则：对一个监视器锁的解锁，happens- before 于随后对这个监视器锁的加锁。
   * volatile变量规则：对一个volatile域的写，happens- before 于任意后续对这个volatile域的读。
   * 传递性：如果A happens- before B，且B happens- before C，那么A happens- before C。

两个操作之间具有happens-before关系，并不意味着前一个操作必须要在后一个操作之前执行！happens-before仅仅要求前一个操作（执行的结果）对后一个操作可见，且前一个操作按顺序排在第二个操作之前（the first is visible to and ordered before the second）

 

happens-before与JMM的关系如下图所示：

如上图所示，一个happens-before规则通常对应于多个编译器重排序规则和处理器重排序规则。对于java程序员来说，happens-before规则简单易懂，它避免程序员为了理解JMM提供的内存可见性保证而去学习复杂的重排序规则以及这些规则的具体实现。 
既然现代处理器，都会有mesi协议保证数据一致性，为何在你举的第一个例子里，会存在数据可见性问题呢，突然想到~



mesi协议保证缓存一致性而非数据一致性。
《Computer Architecture: A Quantitative Approach, 4th Edition》的4.2章，对缓存一致性有详细的说明。



我举的第一个例子（关于写缓冲区），来自于参考文献8的8.2.3.5。在这一章节的最后，有下面这段话：
XXX
上面这段话的大意是说：
事实上，这个例子能够出现这样的结果，是因为store-buffer forwarding。当一个写操作被临时保存在一个处理器的写缓存区时，这个写操作能被该处理器自己读取到；但这个写操作对其他处理器不可见，也不能被其他处理器读取到。


通过使用本地内存这个概念来抽象CPU，内存系统和编译器的优化




数据依赖分下列三种类型：
名称代码示例说明写后读a = 1;b = a;写一个变量之后，再读这个位置。写后写a = 1;a = 2;写一个变量之后，再写这个变量。读后写a = b;b = 1;读一个变量之后，再写这个变量。
上面三种情况，只要重排序两个操作的执行顺序，程序的执行结果将会被改变。
前面提到过，编译器和处理器可能会对操作做重排序。编译器和处理器在重排序时，会遵守数据依赖性，编译器和处理器不会改变存在数据依赖关系的两个操作的执行顺序。 
as-if-serial语义的意思指：不管怎么重排序（编译器和处理器为了提高并行度），（单线程）程序的执行结果不能被改变。编译器，runtime 和处理器都必须遵守as-if-serial语义。




jmm规范对数据竞争的定义如下：在一个线程中写一个变量，在另一个线程读同一个变量，而且写和读没有通过同步来排序。
如果程序是正确同步的，程序的执行将具有顺序一致性（sequentially consistent）--即程序的执行结果与该程序在顺序一致性内存模型中的执行结果相同




顺序一致性内存模型有两大特性：

   * 一个线程中的所有操作必须按照程序的顺序来执行。
   * （不管程序是否同步）所有线程都只能看到一个单一的操作执行顺序。在顺序一致性内存模型中，每个操作都必须原子执行且立刻对所有线程可见。

在JMM中，临界区内的代码可以重排序（但JMM不允许临界区内的代码“逸出”到临界区之外，那样会破坏监视器的语义）



对于未同步或未正确同步的多线程程序，JMM只提供最小安全性：线程执行时读取到的值，要么是之前某个线程写入的值，要么是默认值（0，null，false），JMM保证线程读操作读取到的值不会无中生有（out of thin air）的冒出来。为了实现最小安全性，JVM在堆上分配对象时，首先会清零内存空间，然后才会在上面分配对象（JVM内部会同步这两个操作）。因此，在以清零的内存空间（pre-zeroed memory）分配对象时，域的默认初始化已经完成了。



这两个32位的读/写操作可能会被分配到不同的总线事务中执行，此时对这个64位变量的读/写将不具有原子性。


我记得创建一个对象有三个步骤：1、分配内存；2、将这块内存分配给变量；3、执行构造函数。这里2、3两步的顺序是无序的，所以会产生“双重检查锁定”的问题，在JDK5及之后的版本可以把对象声明为volatile避免，因为volatile语义规定了对volatile变量的读操作必须在写操作之后执行(前提是写操作在时间上是在读操作之前)，以避免无序产生不一致行为。
如果对象变量不声明为volatile，则在多线程环境下也会出现一个“凭空、无效”的引用，JMM保证不了这个安全性。



volatile变量自身具有下列特性：

   * 可见性。对一个volatile变量的读，总是能看到（任意线程）对这个volatile变量最后的写入。
   * 原子性：对任意单个volatile变量的读/写具有原子性，但类似于volatile++这种复合操作不具有原子性。





   * 当写一个volatile变量时，JMM会把该线程对应的本地内存中的共享变量刷新到主内存。
   * 当读一个volatile变量时，JMM会把该线程对应的本地内存置为无效。线程接下来将从主内存中读取共享变量。




下面是JMM针对编译器制定的volatile重排序规则表：
是否能重排序 第二个操作第一个操作普通读/写 volatile读 volatile写 普通读/写  NOvolatile读NONONOvolatile写 NONO
举例来说，第三行最后一个单元格的意思是：在程序顺序中，当第一个操作为普通变量的读或写时，如果第二个操作为volatile写，则编译器不能重排序这两个操作。
从上表我们可以看出：

   * 当第二个操作是volatile写时，不管第一个操作是什么，都不能重排序。这个规则确保volatile写之前的操作不会被编译器重排序到volatile写之后。
   * 当第一个操作是volatile读时，不管第二个操作是什么，都不能重排序。这个规则确保volatile读之后的操作不会被编译器重排序到volatile读之前。
   * 当第一个操作是volatile写，第二个操作是volatile读时，不能重排序。


为了实现volatile的内存语义，编译器在生成字节码时，会在指令序列中插入内存屏障来禁止特定类型的处理器重排序。对于编译器来说，发现一个最优布置来最小化插入屏障的总数几乎不可能，为此，JMM采取保守策略。下面是基于保守策略的JMM内存屏障插入策略：

   * 在每个volatile写操作的前面插入一个StoreStore屏障。
   * 在每个volatile写操作的后面插入一个StoreLoad屏障。
   * 在每个volatile读操作的后面插入一个LoadLoad屏障。
   * 在每个volatile读操作的后面插入一个LoadStore屏障。



在JSR-133之前的旧Java内存模型中，虽然不允许volatile变量之间重排序，但旧的Java内存模型允许volatile变量与普通变量之间重排序。



因为处理器总线会同步多个处理器对内存的并发访问（请参阅我在这个系列的第三篇中，对处理器总线工作机制的描述）。
因此在任意时间点，不可能有两个线程能同时更新X的值。


--加在数组引用上的volatile可以保证任意线程都能看到这个数组引用的最新值（但不保证数组元素的可见性）。 



当线程释放锁时，JMM会把该线程对应的本地内存中的共享变量刷新到主内存中。

当线程获取锁时，JMM会把该线程对应的本地内存置为无效。从而使得被监视器保护的临界区代码必须要从主内存中去读取共享变量。



inline jint     Atomic::cmpxchg    (jint     exchange_value, volatile jint*     dest, jint     compare_value) {  // alternative for InterlockedCompareExchange  int mp = os::is_MP();  __asm {    mov edx, dest    mov ecx, exchange_value    mov eax, compare_value    LOCK_IF_MP(mp)    cmpxchg dword ptr [edx], ecx  }} 


  1. 
intel的手册对lock前缀的说明如下：


由于在指令执行期间该缓存行会一直被锁定，其它处理器无法读/写该指令要访问的内存区域，因此能保证指令执行的原子性。这个操作过程叫做缓存锁定（cache locking），缓存锁定将大大降低lock前缀指令的执行开销，但是当多处理器之间的竞争程度很高或者指令访问的内存地址未对齐时，仍然会锁住总线。
  2. 禁止该指令与之前和之后的读和写指令重排序。
  3. 把写缓冲区中的所有数据刷新到内存中。


concurrent包的源代码实现，会发现一个通用化的实现模式：

  1. 首先，声明共享变量为volatile；
  2. 然后，使用CAS的原子条件更新来实现线程之间的同步；
  3. 同时，配合以volatile的读/写和CAS所具有的volatile读和写的内存语义来实现线程之间的通信。



memory = allocate();   //1：分配对象的内存空间ctorInstance(memory);  //2：初始化对象instance = memory;     //3：设置instance指向刚分配的内存地址
上面三行伪代码中的2和3之间，可能会被重排序.B线程将看到一个还没有被初始化的对象。 


线程优势
l 创建时间开销远小于进程的创建。因为不需要分配用户空间和那么多初始化动作。
l 销毁线程的成本也远低于进程。
l 线程之间的切换消耗低于进程，特别是同一进程内的线程切换消耗更低。
l 线程间通信的效率比进程间通信要高，因为进城之间安全性问题需要隔离和互斥，同一进程内的线程可以共享进程资源而不需要提前获取锁。 

JAVA 内存模型屏蔽掉各种硬件和操作系统的内存访问差异，以实现JAVA在各个平台都达到一致的并发效果。其主要目标是定义程序中各个变量的访问规则，变量存储到内存以及取出的各种细节。 抽象出工作内存和主内存。 工作内存可以理解为cpu的缓冲等概念。线程对变量的操作必须在工作内存里面进行。


定义8种操作来完成变量存储到内存以及取出的各种细节细节：
lock  锁住主存变量，线程独享 
unlock 解锁主存变量，对应lock 
read  从主内存中读取变量到工作内存（线程内存） 
load  从工作内存中读取read到的变量赋值给工作内存中的变量副本。 
use   执行引擎使用到工作内存的变量（读取） 
assign 执行引擎对变量的修改后赋值给工作内存变量（改变） 
store  把工作内存中的变量放入到主内存中 
write  把从工作内存中store得到的变量赋值给主内存变量。 


1，成对出现。比如 read 和 write， load 和 store 
2，确保assign后调用store，不允许没有assign操作，而store 
3，新变量只能在主存中产生。也就是 use 和 store之前，必须有 load 和 assign 
4，一次只能有一个线程lock操作，同一个线程可以多次lock（可重入） 
5，lock之前，必须同步变量到主存。store 和 write 
6，lock清空工作内存中的变量，重新 load 或者 assign 
7，unlock不能单独使用，必须先有lock。 


volatile 保证可见性，禁止重排序


前面定义了各种操作，是为了保证 原子性，可见性，有序性。


happens before ，先行发生原则用来判断 数据是否发生竞争，线程是否安全的重要依据。 


1，（Program Order Rule）在同一个线程中，按代码顺序执行。也就是根据控制流，执行到哪里就是哪里。 
2，（Monitor Lock Rule） unlock 操作先行发生后面的 lock ，针对同一个锁。 
3，（Volatile Variable Rule) 对于一个volatileo变量的写先于后面对这个变量的读。 
4，（Thread start Rule） 线程的start方法先行发生与此线程的其他动作 
5，（Thread Termination Rule）线程的所有操作都优先于对此线程的终止检测。 
6，（Thread Interruption Rule）对interrupt()方法的调用先行发生于被中断线程的代码检测到中断事件的发生。 
7，（Finalizer Rule） 一个对象的初始化优先与finalize方法调用 
8，（Transitivity） A优先与B ， B 优先于 C ， 则A 优先于 C 


进程是程序运行的实例，线程是cpu的基本调度单位。线程把进程的资源分配和调度执行分开。线程共享了进程的内存地址，文件句柄等，又可以独立调度。


自旋锁+pause+超过次数仍未成功转成传统方式锁+自适应调整（根据监控数据是增加自旋时间还是使用传统锁）
锁消除（synchronized new object ,local stringbuffer） 粗化(string buffer 多次append) 偏向锁，轻量级锁



 
### CAS

### FalseSharing


先第一维，后第二维 cacheline
 
  1. longs = new long[DIMENSION_1][];  
  2.         for (int i = 0; i < DIMENSION_1; i++) {  
  3.             longs[i] = new long[DIMENSION_2];  
  4.             for (int j = 0; j < DIMENSION_2; j++) {  
  5.                 longs[i][j] = 0L;  
  6.             }  
  7.         }  

远程写(Remote Write) 其实确切地说不是远程写, 而是c2得到c1的数据后, 不是为了读, 而是为了写. 也算是本地写, 只是c1也拥有这份数据的拷贝, 这该怎么办呢? c2将发出一个RFO(Request For Owner)请求, 它需要拥有这行数据的权限, 其它处理器的相应缓存行设为I, 除了它自已, 谁不能动这行数据. 这保证了数据的安全, 同时处理RFO请求以及设置I的过程将给写操作带来很大的性能消耗. 

伪共享 
我们从上节知道, 写操作的代价很高, 特别当需要发送RFO消息时. 我们编写程序时, 什么时候会发生RFO请求呢? 有以下两种: 
1. 线程的工作从一个处理器移到另一个处理器, 它操作的所有缓存行都需要移到新的处理器上. 此后如果再写缓存行, 则此缓存行在不同核上有多个拷贝, 需要发送RFO请求了. 
2. 两个不同的处理器确实都需要操作相同的缓存行 


一个运行在处理器core 1上的线程想要更新变量X的值, 同时另外一个运行在处理器core 2上的线程想要更新变量Y的值. 但是, 这两个频繁改动的变量都处于同一条缓存行. 两个线程就会轮番发送RFO消息, 占得此缓存行的拥有权. 当core 1取得了拥有权开始更新X, 则core 2对应的缓存行需要设为I状态. 当core 2取得了拥有权开始更新Y, 则core 1对应的缓存行需要设为I状态(失效态). 轮番夺取拥有权不但带来大量的RFO消息, 而且如果某个线程需要读此行数据时, L1和L2缓存上都是失效数据, 只有L3缓存上是同步好的数据.从前一篇我们知道, 读L3的数据非常影响性能. 更坏的情况是跨槽读取, L3都要miss,只能从内存上加载. 
表面上X和Y都是被独立线程操作的, 而且两操作之间也没有任何关系.只不过它们共享了一个缓存行, 但所有竞争冲突都是来源于共享. 


Acquire语义限制了编译器优化、CPU乱序，不能将含有Acquire语义的操作之后的代码，提到含有Acquire语义的操作代码之前执行；
Release语义限制了编译器优化、CPU乱序，不能将含有Release语义的操作之前的代码，推迟到含有Release语义的操作代码之后执行；


原子（atom）本意是“不能被进一步分割的最小粒子”，而原子操作（atomic operation）意为"不可被中断的一个或一系列操作” 。

假共享是指多个CPU同时修改同一个缓存行的不同部分而引起其中一个CPU的操作无效，



多个处理器同时从各自的缓存中读取变量i，分别进行加一操作，然后分别写入系统内存当中。



所谓“缓存锁定”就是如果缓存在处理器缓存行中内存区域在LOCK操作期间被锁定，当它执行锁操作回写内存时，处理器不在总线上声言LOCK＃信号，而是修改内部的内存地址，并允许它的缓存一致性机制来保证操作的原子性，因为缓存一致性机制会阻止同时修改被两个以上处理器缓存的内存区域数据，当其他处理器回写已被锁定的缓存行的数据时会起缓存行无效，



但是有两种情况下处理器不会使用缓存锁定。第一种情况是：当操作的数据不能被缓存在处理器内部，或操作的数据跨多个缓存行（cache line），则处理器会调用总线锁定。第二种情况是：有些处理器不支持缓存锁定。对于Inter486和奔腾处理器,就算锁定的内存区域在处理器的缓存行中也会调用总线锁定。



但是CAS仍然存在三大问题。ABA问题，循环时间长开销大和只能保证一个共享变量的原子操作。



ABA问题的解决思路就是使用版本号。在变量前面追加上版本号，每次变量更新的时候把版本号加一，那么A－B－A 就会变成1A-2B－3A。从Java1.5开始JDK的atomic包里提供了一个类AtomicStampedReference来解决ABA问题。这个类的compareAndSet方法作用是首先检查当前引用是否等于预期引用，并且当前标志是否等于预期标志，如果全部相等，则以原子方式将该引用和该标志的值设置为给定的更新值。



public boolean compareAndSet        (V      expectedReference,//预期引用         V      newReference,//更新后的引用        int    expectedStamp, //预期标志        int    newStamp) //更新后的标志循环时间长开销大。自旋CAS如果长时间不成功，会给CPU带来非常大的执行开销。如果JVM能支持处理器提供的pause指令那么效率会有一定的提升，pause指令有两个作用，第一它可以延迟流水线执行指令（de-pipeline）,使CPU不会消耗过多的执行资源，延迟的时间取决于具体实现的版本，在一些处理器上延迟时间是零。第二它可以避免在退出循环的时候因内存顺序冲突（memory order violation）而引起CPU流水线被清空（CPU pipeline flush），从而提高CPU的执行效率。 
从Java1.5开始JDK提供了AtomicReference类来保证引用对象之间的原子性，你可以把多个变量放在一个对象里来进行CAS操作。




使用线程中断

   * JVM不保证阻塞方法检测到中断的速度,但在实际应用响应速度还是很快的。
   * 当线程处于下列方法的调用时，interrupt方法会清除中断状态,并抛出中断异常
      * 也就方法声明直接抛出InterruptedException的:Object的wait(), wait(long), or wait(long, int) 方法, Thread的join(), join(long), join(long, int), sleep(long), 或者 sleep(long, int)
   * 当线程下列情形时，interrupt方法会设置中断状态,但是并不抛出中断异常
      * 在 interruptible channel进行IO操作时, 该channel会被关闭,然后抛出java.nio.channels.ClosedByInterruptException.
      * 线程阻塞在java.nio.channels.Selector 时,则线程会立即返回一个非0的值.就好像调用了该Selector的wakeup方法.
      * 其他未说明的情况(比如在catch IE后,需要传递中断状态,则调用Thread.currentThread.interrupt(),但是此时不满足上述几种情况,则仅仅是设置线程中断状态,不会抛出IE异常)
   * 传递InterruptedException
      * 根本不捕获异常
      * 捕获该异常,简单处理后,再次抛出该异常
   * 恢复中断
   * 
      * 由于jvm在抛出IE异常后,会自动将中断状态置为false.所以为了调用栈中更高层的看到这个中断,则需要执行Thread.currentThread.interrupt()，这样做仅仅是为了设置线程中断状态为true.

修复多线程同步不当的问题

   * 
      * 不在线程之间共享状态变量值
      * 将状态变量修改为不可变的变量
      * 在访问状态变量时使用同步

设计线程安全类的三要素

   * 
      * 找出构成对象状态的所有要素(看类的属性)
      * 找出约束状态变量的不变性条件(也称不变式，比如状态之间的关联关系,具体例如Produce/Consume,队列满时不能放,队列空时不能取等)
      * 建立对象状态的并发访问管理策略


   * 用户线程和守护线程两者几乎没有区别，唯一的不同之处就在于虚拟机的离开：如果用户线程已经全部退出运行了，只剩下守护线程存在了，虚拟机也就退出了。 因为没有了被守护者，守护线程也就没有工作可做了，也就没有继续运行程序的必要了。

### 为什么要同步

   * 编译器，处理器以及内存系统可能会让两条语句的机器指令交错。比如在32位机器上，b变量的高位字节先被写入，然后是a变量，紧接着才会是b变量的低位字节。
   * 编译器，处理器，内存系统 可能会进行指令重排序


何登成  原子操作  汇编认识  monitor enter  可以reorder，但是不能对外可见  exit 内部操作对外可见

 
 
局部性原理 
 

由于 CS 
内存取值

插入一个内存屏障，相当于告诉CPU和编译器先于这个命令的必须先执行，后于这个命令的必须后执行。内存屏障另一个作用是强制更新一次不同CPU的缓存。例如，一个写屏障会把这个屏障前写入的数据刷新到缓存，这样任何试图读取该数据的线程将得到最新值，而不用考虑到底是被哪个cpu核心或者哪颗CPU执行的


### Java中是如何支持多线程的
#### JMM
上下文切换是存储和恢复CPU状态的过程，它使得线程执行能够从中断点恢复执行。上下文切换是多任务操作系统和多线程环境的基本特征。
要理解happens-before必须先理解重排序。happens-before就是对重排序的限制，即哪些情况下是不能重排序的。

happens-before是比重排序、内存屏障这些概念更上层的东西，我们没有办法穷举在所有的CPU/计算机架构下重排序会如何发生，也没办法为这些重排序清晰的定义该在什么时候插入屏障来阻止重排序、刷新cache的顺序等，如果java语言提供屏障操作让java开发者自己决定何时插入屏障，那么并发代码将非常难写还很容易出错，而hb规则恰好向Java开发者屏蔽了这些特定平台的底层细节，VM的开发者遵守hb规则（他们在开发某个平台上的VM时是清晰地知道在遇到某条规则时该在哪里插入内存屏障的），Java开发者也遵守hb规则（他们知道遵守了规定就能得到想要的结果），就能保证可见性。
 
但是同步的含义比互斥更广。同步保证了一个线程在同步块之前或者在同步块中的一个内存写入操作以可预知的方式对其他有相同监视器的线程可见。当我们退出了同步块，我们就释放了这个监视器，这个监视器有刷新缓冲区到主内存的效果，因此该线程的写入操作能够为其他线程所见。在我们进入一个同步块之前，我们需要获取监视器，监视器有使本地处理器缓存失效的功能，因此变量会从主存重新加载，于是其它线程对共享变量的修改对当前线程来说就变得可见了。

在旧的内存模型下，访问volatile变量不能被重排序，但是，它们可能和访问非volatile变量一起被重排序。这破坏了volatile字段从一个线程到另外一个线程作为一个信号条件的手段。

Java内存模型描述了在多线程代码中哪些行为是合法的，以及线程如何通过内存进行交互。它描述了“程序中的变量“ 和 ”从内存或者寄存器获取或存储它们的底层细节”之间的关系。Java内存模型通过使用各种各样的硬件和编译器的优化来正确实现以上事情。
Java包含了几个语言级别的关键字，包括：volatile, final以及synchronized，目的是为了帮助程序员向编译器描述一个程序的并发需求。Java内存模型定义了volatile和synchronized的行为，更重要的是保证了同步的java程序在所有的处理器架构下面都能正确的运行。

 Java包含了几个语言级别的关键字，包括：volatile, final以及synchronized，目的是为了帮助程序员向编译器描述一个程序的并发需求。Java内存模型定义了volatile和synchronized的行为，更重要的是保证了同步的java程序在所有的处理器架构下面都能正确的运行。


  1. 



更确切地说是硬件对内存访问实施了优化。一般来说，CPU指令执行的速度比从主存读取数据的速度要快2到3个数量级。显然内存子系统是整个系统的屏颈，硬件工程师使尽浑身解数想出聪明办法来使访问内存更快。首先是使用cache来加速内存访问，然而这带来了下面这些额外的复杂性：

  1. 当cache访问不命中时，处理仍然难逃被内存子系统拖慢的厄运。
  2. 在多处理器系统，必须使用协议保存cache一致性。

Store Buffer
当处理器所读取的内存是多处理器系统的共享内存时，事情变得更复杂。必须使用协议来保证，当某变量的最新值保存到CPU的cache时，其它所有CPU的cache上该变量的副本必须更改成无效状态，以在所有处理器上保持值的一致性。这种协议的缺点是CPU在写数据时，不可避免地受到了拖延。
硬件工程再度想出聪明的解决方法：将写请求缓冲到一个称为store buffer的特殊硬件队列。所有请求都放到队列里，随后CPU方便时一下子将修改请求应用内存里。
对于软件开发人员，更关心的问题时，何时谓之方便。上面的marathon程序可能会发生这样的场景，‘arrived=true‘请求已排队到store buffer，但store buffer上的请求永远都不对主存生效。因此线程A永远也看不到标志变量的新值。Oops!……

内存屏障是一种特殊的处理器指令，它指挥处理器做如下的事情：

   * 刷新store buffer。
   * 等待直到内存屏障之前的操作已经完成。
   * 不将内存屏障后面的指令提前到内存屏障之前执行


#### 同步关键字

synchronized 表达的语义表达的有3个：
 
  1. 原子：即通过互斥保证同时只有一个线程能够获取锁，保证数据的原子性
  2. 内存可见：即离开同步块后，临界区内的操作都已经刷新到内存里面
  3. 有序：同步块里面的代码块内部执行可以乱序，但是必须保证进入同步块和离开同步块的代码不能被乱序执行。

把变量声明为volatile类型后,编译器和运行时都会注意到这个变量是共享的,因此不会将该变量上的操作和其他内存操作进行重排序.volatile变量不会被缓存在寄存器或者处理器其他不可见的地方,因此在读取volatile类型的变量总是会返回最新写入的值
 

对final域，编译器和处理器要遵守两个重排序规则：
1. 在构造函数内对一个final域的写入，与随后把这个被构造对象的引用赋值给一个引用变量，这两个操作之间不能重排序。2. 初次读一个包含final域的对象的引用，与随后初次读这个final域，这两个操作之间不能重排序。
用大白话表达的话，就是说

  1. 在创建某个对象时，如果该对象含有final属性，那么这个final属性值必须在构造方法内完成初始化赋值，然后才能给外部对象引用。写final域的重排序规则可以确保：在对象引用为任意线程可见之前，对象的final域已经被正确初始化过了，而普通域不具有这个保障。
  2. 读final域的重排序规则可以确保：在读一个对象的final域之前，一定会先读包含这个final域的对象的引用。
  3. 对于引用类型，写final域的重排序规则对编译器和处理器增加了如下约束：在构造函数内对一个final引用的对象的成员域的写入，与随后在构造函数外把这个被构造对象的引用赋值给一个引用变量，这两个操作之间不能重排序。



volatile，atomic，lock、cas，synchronized，


stop会导致对象状态不一致。而suspend和resume确实会导致死锁。


中断，检查volatile 中断标识，毒丸（也是检查标识）
只需要保证共享资源的可见性的时候可以使用volatile替代，synchronized保证可操作的原子性一致性和可见性。
volatile适用于新值不依赖于就值的情形。
个volatile字段很适合作为完成某些工作的标志，
可以使用volatile的情况包括：



   * 该字段不遵循其他字段的不变式。
   * 对字段的写操作不依赖于当前值。
   * 没有线程违反预期的语义写入非法值。
   * 读取操作不依赖于其它非volatile字段的值。

#### 线程通讯

#### JUC类库



要实现一个线程安全的队列有两种实现方式：一种是使用阻塞算法，另一种是使用非阻塞算法。使用阻塞算法的队列可以用一个锁（入队和出队用同一把锁）或两个锁（入队和出队用不同的锁）等方式来实现，而非阻塞的实现方式则可以使用循环CAS的方式来实现，




阻塞队列提供了四种处理方法:
方法\处理方式抛出异常返回特殊值一直阻塞超时退出插入方法add(e)offer(e)put(e)offer(e,time,unit)移除方法remove()poll()take()poll(time,unit)检查方法element()peek()不可用不可用




   * ArrayBlockingQueue ：一个由数组结构组成的有界阻塞队列。
   * LinkedBlockingQueue ：一个由链表结构组成的有界阻塞队列。
   * PriorityBlockingQueue ：一个支持优先级排序的无界阻塞队列。
   * DelayQueue：一个使用优先级队列实现的无界阻塞队列。
   * SynchronousQueue：一个不存储元素的阻塞队列。
   * LinkedTransferQueue：一个由链表结构组成的无界阻塞队列。
   * LinkedBlockingDeque：一个由链表结构组成的双向阻塞队列。


LinkedBlockingQueue是一个用链表实现的有界阻塞队列。此队列的默认和最大长度为Integer.MAX_VALUE。此队列按照先进先出的原则对元素进行排序。
PriorityBlockingQueue是一个支持优先级的无界队列。默认情况下元素采取自然顺序排列，也可以通过比较器comparator来指定元素的排序规则。元素按照升序排列。
DelayQueue是一个支持延时获取元素的无界阻塞队列。队列使用PriorityQueue来实现。队列中的元素必须实现Delayed接口，在创建元素时可以指定多久才能从队列中获取当前元素。只有在延迟期满时才能从队列中提取元素。我们可以将DelayQueue运用在以下应用场景：

   * 缓存系统的设计：可以用DelayQueue保存缓存元素的有效期，使用一个线程循环查询DelayQueue，一旦能从DelayQueue中获取元素时，表示缓存有效期到了。
   * 定时任务调度。使用DelayQueue保存当天将会执行的任务和执行时间，一旦从DelayQueue中获取到任务就开始执行，从比如TimerQueue就是使用DelayQueue实现的。




SynchronousQueue是一个不存储元素的阻塞队列。每一个put操作必须等待一个take操作，否则不能继续添加元素。SynchronousQueue可以看成是一个传球手，负责把生产者线程处理的数据直接传递给消费者线程。队列本身并不存储任何元素，非常适合于传递性场景,比如在一个线程中使用的数据，传递给另外一个线程使用，SynchronousQueue的吞吐量高于LinkedBlockingQueue 和 ArrayBlockingQueue。
LinkedTransferQueue是一个由链表结构组成的无界阻塞TransferQueue队列。相对于其他阻塞队列，LinkedTransferQueue多了tryTransfer和transfer方法。
transfer方法。如果当前有消费者正在等待接收元素（消费者使用take()方法或带时间限制的poll()方法时），transfer方法可以把生产者传入的元素立刻transfer（传输）给消费者。如果没有消费者在等待接收元素，transfer方法会将元素存放在队列的tail节点，并等到该元素被消费者消费了才返回。 


使用通知模式实现。所谓通知模式，就是当生产者往满的队列里添加元素时会阻塞住生产者，当消费者消费了一个队列中的元素后，会通知生产者当前队列可用。




park这个方法会阻塞当前线程，只有以下四种情况中的一种发生时，该方法才会返回。

   * 与park对应的unpark执行或已经执行时。注意：已经执行是指unpark先执行，然后再执行的park。
   * 线程被中断时。
   * 如果参数中的time不是零，等待了指定的毫秒数时。
   * 发生异常现象时。这些异常事先无法确定。



Fork/Join框架是Java7提供了的一个用于并行执行任务的框架， 是一个把大任务分割成若干个小任务，最终汇总每个小任务结果后得到大任务结果的框架。

 ？假如我们需要做一个比较大的任务，我们可以把这个任务分割为若干互不依赖的子任务，为了减少线程间的竞争，于是把这些子任务分别放到不同的队列里，并为每个队列创建一个单独的线程来执行队列里的任务，线程和队列一一对应，比如A线程负责处理A队列里的任务。但是有的线程会先把自己队列里的任务干完，而其他线程对应的队列里还有任务等待处理。干完活的线程与其等着，不如去帮其他线程干活，于是它就去其他线程的队列里窃取一个任务来执行。而在这时它们会访问同一个队列，所以为了减少窃取任务线程和被窃取任务线程之间的竞争，通常会使用双端队列，被窃取任务线程永远从双端队列的头部拿任务执行，而窃取任务的线程永远从双端队列的尾部拿任务执行。




如果让我们来设计一个Fork/Join框架，该如何设计？这个思考有助于你理解Fork/Join框架的设计。
第一步分割任务。首先我们需要有一个fork类来把大任务分割成子任务，有可能子任务还是很大，所以还需要不停的分割，直到分割出的子任务足够小。 
第二步执行任务并合并结果。分割的子任务分别放在双端队列里，然后几个启动线程分别从双端队列里获取任务执行。子任务执行完的结果都统一放在一个队列里，启动一个线程从队列里拿数据，然后合并这些数据。
Fork/Join使用两个类来完成以上两件事情：

   * ForkJoinTask：我们要使用ForkJoin框架，必须首先创建一个ForkJoin任务。它提供在任务中执行fork()和join()操作的机制，通常情况下我们不需要直接继承ForkJoinTask类，而只需要继承它的子类，Fork/Join框架提供了以下两个子类：
      * RecursiveAction：用于没有返回结果的任务。
      * RecursiveTask ：用于有返回结果的任务。
   * ForkJoinPool ：ForkJoinTask需要通过ForkJoinPool来执行，任务分割出的子任务会添加到当前工作线程所维护的双端队列中，进入队列的头部。当一个工作线程的队列里暂时没有任务时，它会随机从其他工作线程的队列的尾部获取一个任务。


AtomicStampedReference ABA ABA问题可以使用AtomicStampedReference在做CAS操作时，一方面比较内存中的操作数与预期值是否一样，同时比较内存中的操作数的时间戳（或者修改次数）是否与预期值一样。如果本次修改操作成功，一定要修改操作数的时间戳，可以通过每次加1的方式。


原子性（lock前缀，cas），内存屏障（loadstore 等4个内存屏障） 有序性、可见性
  1. cpu，编译器重排序

工作内存，主内存的可见性  打印汇编 打印类加载情况



try-finally中使用。


CAS读取内存值
比较内存值和期望值
替换内存值为要替换值
读写锁支持多个读操作并发执行，写操作只能由一个线程来操作

在一个Executor对象中使用我们的ThreadFactory

ThreadPoolExecutor



 Semaphore本身是用来限制有限资源的争用。比如我要限制数据数据库连接池的连接数不超过20个，我可以在连接池的配置里面设置最大值，



1 ReentrantLock 相对于固有锁synchronized，同样是可重入的，在某些vm版本上提供了比固有锁更高的性能，提供了更丰富的锁特性，比如可中断的锁，可等待的锁，平等锁以及非块结构的加锁。从代码上尽量用固有锁，vm会对固有锁做一定的优化，并且代码可维护和稳定。只有在需要ReentrantLock的一些特性时，可以考虑用ReentrantLock实现。

 synchronized无法中断一个正在等待获得锁的线程，也即多线程竞争一个锁时，其余未得到锁的线程只能不停的尝试获得锁，而不能中断

同步和互斥，资源互斥、协调竞争是要解决的因，同步是竞争协调的果。可以使用synchnized/notify/notifyAll以及Lock/Condition, CyclicBarrier/Semaphore/Countdownbatch。线程的join以及Future/FutureTask是为了解决多线程计算后的结果统计


ConcurrentHashMap是由Segment数组结构和HashEntry数组结构组成。Segment是一种可重入锁ReentrantLock，在ConcurrentHashMap里扮演锁的角色，HashEntry则用于存储键值对数据。一个ConcurrentHashMap里包含一个Segment数组，Segment的结构和HashMap类似，是一种数组和链表结构， 一个Segment里包含一个HashEntry数组，每个HashEntry是一个链表结构的元素， 每个Segment守护者一个HashEntry数组里的元素,当对HashEntry数组的数据进行修改时，必须首先获得它对应的Segment锁。

### 线程池

   * 线程池是先指定core size（prestartall）,然后填满队列,最后再到max size。所以要注意参数的合理设置。
   * 没指定默认队列大小。
   * 在线程池情况下，没有在finally中调用ThreadLocal#remove方法,或者ThreadLocal不是static变量
   * 需要注意在使用await()和signal()方法时，如果你在condition上调用await()方法而却没有在这个condition上调用signal()方法，这个线程将永远睡眠下去

拒绝策略

   * new  ThreadPoolExecutor(corePoolSize, maximumPoolSize, keepAliveTime, milliseconds,runnableTaskQueue, handler);
public void execute(Runnable command) {
Future<Object> future = executor.submit(harReturnValuetask);
 V Callable.call() throws Exception; 
   * 

   * ArrayBlockingQueue：是一个基于数组结构的有界阻塞队列，此队列按 FIFO（先进先出）原则对元素进行排序。
   * LinkedBlockingQueue：一个基于链表结构的阻塞队列，此队列按FIFO （先进先出） 排序元素，吞吐量通常要高于ArrayBlockingQueue。静态工厂方法Executors.newFixedThreadPool()使用了这个队列。
   * SynchronousQueue：一个不存储元素的阻塞队列。每个插入操作必须等到另一个线程调用移除操作，否则插入操作一直处于阻塞状态，吞吐量通常要高于LinkedBlockingQueue，静态工厂方法Executors.newCachedThreadPool使用了这个队列。
   * PriorityBlockingQueue：一个具有优先级的无限阻塞队列。



   * AbortPolicy：直接抛出异常。
   * CallerRunsPolicy：只用调用者所在线程来运行任务。
   * DiscardOldestPolicy：丢弃队列里最近的一个任务，并执行当前任务。
   * DiscardPolicy：不处理，丢弃掉。
   * 当然也可以根据应用场景需要来实现RejectedExecutionHandler接口自定义策略。如记录日志或持久化不能处理的任务。



   * 监控：taskCount：线程池需要执行的任务数量。
   * completedTaskCount：线程池在运行过程中已完成的任务数量。小于或等于taskCount。
   * largestPoolSize：线程池曾经创建过的最大线程数量。通过这个数据可以知道线程池是否满过。如等于线程池的最大大小，则表示线程池曾经满了。
   * getPoolSize:线程池的线程数量。如果线程池不销毁的话，池里的线程不会自动销毁，所以这个大小只增不+ getActiveCount：获取活动的线程数。


长度
内容
说明
32/64bit
Mark Word
存储对象的hashCode或锁信息等。
32/64bit
Class Metadata Address
存储到对象类型数据的指针
32/64bit
Array length
数组的长度（如果当前对象是数组）


无锁状态，偏向锁状态，轻量级锁状态和重量级锁状态

Volatile比synchronized的使用和执行成本会更低，因为它不会引起线程上下文的切换和调度。




锁
优点
缺点
适用场景
偏向锁
加锁和解锁不需要额外的消耗，和执行非同步方法比仅存在纳秒级的差距。
如果线程间存在锁竞争，会带来额外的锁撤销的消耗。
适用于只有一个线程访问同步块场景。
轻量级锁
竞争的线程不会阻塞，提高了程序的响应速度。
如果始终得不到锁竞争的线程使用自旋会消耗CPU。
追求响应时间。
同步块执行速度非常快。
重量级锁
线程竞争不使用自旋，不会消耗CPU。
线程阻塞，响应时间缓慢。
追求吞吐量。
同步块执行速度较长。

lock前缀的指令在多核处理器下会引发了两件事情。

   * 将当前处理器缓存行的数据会写回到系统内存。 lock addl $0x0,(%esp);一般不锁总线，而是锁缓存。
   * 这个写回内存的操作会引起在其他CPU里缓存了该内存地址的数据无效。MESI（修改，独占，共享，无效）

一个对象的引用占4个字节，它追加了15个变量共占60个字节，再加上父类的Value变量，一共64个字节。



为什么追加64字节能够提高并发编程的效率呢？ 因为对于英特尔酷睿i7，酷睿， Atom和NetBurst， Core Solo和Pentium M处理器的L1，L2或L3缓存的高速缓存行是64个字节宽，不支持部分填充缓存行，这意味着如果队列的头节点和尾节点都不足64字节的话，处理器会将它们都读到同一个高速缓存行中，在多处理器下每个处理器都会缓存同样的头尾节点，当一个处理器试图修改头接点时会将整个缓存行锁定，那么在缓存一致性机制的作用下，会导致其他处理器不能访问自己高速缓存中的尾节点，而队列的入队和出队操作是需要不停修改头接点和尾节点，所以在多处理器的情况下将会严重影响到队列的入队和出队效率。Doug lea使用追加到64字节的方式来填满高速缓冲区的缓存行，避免头接点和尾节点加载到同一个缓存行，使得头尾节点在修改时不会互相锁定。



在两种场景下不应该使用这种方式。第一：缓存行非64字节宽的处理器，如P6系列和奔腾处理器，它们的L1和L2高速缓存行是32个字节宽。第二：共享变量不会被频繁的写。因为使用追加字节的方式需要处理器读取更多的字节到高速缓冲区，这本身就会带来一定的性能消耗，共享变量如果不被频繁写的话，锁的几率也非常小，就没必要通过追加字节的方式来避免相互锁定。


 

## COLLECTION

基本类库概要说明

   * Iterable
   * Collection
      * List
      * Set
      * Queue
         * BlockingQueue
            * LinkedBlockingQueue
            * ArrayBlockingQueue
            * PriorityBlockingQueue
      * Deque
   * 
Map
      * SortedMap
   * 
ConcurrentHashMap
      * 内部使用了分段锁机制(Lock Striping),这种机制允许任意数量的读取线程可以并发地访问Map,并且允许一定数量的线程可以并发地修改Map
      * CHM返回的迭代器具有弱一致性(Weakly Consistent),而并非"及时失败".弱一致性的迭代器可以容忍并发修改,并可以(但并不保证)在迭代器被构造后将修改操作反映给容器
   * 
CopyOnWriteArrayList
      * 在每次修改时,都会创建并重新发布一个新的容器副本.
      * 适用于读多写少的场景,常用于监听器模式.
   * 
Deque
      * 双端队列,支持在队列头和队列尾实现高速插入和移除
   * 
同步工具类
      * 
CountDownLatch(闭锁)
         * Latch 译为 门闩
         * 简单理解:一开始,该Latch时关闭的.内部有个计数器,每调用一次countDown方法,计数器减一.当计数器为0时,该Latch打开,在latch上await的线程可以继续执行.当Latch打开后,其状态不会再改变.(也就是说,要新建一个CountDownLatch实例才能完成相同功能)
         * 通常用于确保某些活动在其他活动都完成后才继续执行.
         * 实例伪码
     CountDownLatch latch = new CountDownLatch(10);     Thread t = new Runnable({ public void run(){          latch.await;          task.run();     } })     t.start;     latch.countDown();
      * 
CyclicBarrier
         * 所有线程必须都已经到达栅栏(或者await调用超时或者被中断),才能开始继续进行.
         * vs CountDownLatch
            * Barrier 等待所有线程到达栅栏处,比如统计运动员的比赛成绩
            * Latch 主要取决于刚开始计数器的大小以及countDown方法的调用次数
            * Barrier计数器可以重置,Latch内置计数器不可以
      * FutureTask
         * 常用于并行计算
         * 任务状态
            * 等待运行
            * 正在运行
            * 完成运行
               * 任务正常完成结束
               * 任务异常结束
               * 取消任务执行
               * 任务执行超时
         * 使用FutureTask.get
         * 处理异常
      * Semaphore
         * 常用于控制同时访问某个特定资源/执行特定操作的数量(比如连接池)
         * 使用acquire来获得(占用)许可(如果没有许可将阻塞);使用release来释放许可
         * 不可重入


## GC

 * 程序计数器,用来在线程切换后,仍然能够将线程恢复的正确的执行位置.PC的值通常是下一条需要执行的字节码指令.生命周期与线程相同的.
   * Java虚拟机栈,即我们通常所说的栈,主要用来存放栈帧.栈帧用于存储局部变量表(基本数据类型,对象引用和returnAddress类型)、操作栈、动态链接、方法出口等.生命周期与线程相同的.
   * Java堆,几乎所有的对象实例和数组都是在堆上分配的(栈上分配,标量替换使得对象实例不一定分配在堆上面).生命周期与JVM 进程相同
   * 方法区,主要用来存放类信息,常量,静态变量,即时编译后的代码等数据.
   * 方法区：类信息，静态变量，运行时常量池。
   * 运行时常量池,该区是方法区的一部分,主要用于存放从编译期可知的数值字面量到必须运行期解析后才能获得的方法或字段引用.Class文件中的常量池在被类加载后就是存放到方法区的运行时常量池中.
   * 本地方法栈,Java虚拟机实现可能会使用到传统的栈（通常称之为“C Stacks”）来支持native方法（指使用Java以外的其他语言编写的方法）的执行，这个栈就是本地方法栈（Native Method Stack）。

根搜索算法,即 GC Roots Tracing,该算法主要是:以名为"GC Roots"的对象为根节点,从这个根节点向下搜索,所有走过的路径为引用链,当一个对象到GC Roots没有任何引用链相连时,则证明这个对象是不可用的,可以被垃圾回收.主流商用GC语言中,都是选用根搜索算法



软引用,即Soft Reference, 在系统将要发生内存溢出异常之前,将会被这些对象列进回收范围内进行第二次垃圾回收.如果这次回收还是没有得到足够的内存,才会抛出内存溢出异常



弱引用,即Weak Reference,它的强度比软引用还弱一些,只能存活到下一次垃圾回收之前.当垃圾回收时,无论内存是否够用,都会回收掉被弱引用关联的对象



虚引用,即Phantom   [‘fæntəm] Reference,它是最弱的一种引用关系.一个对象是否有虚引用的存在,完全不会对其生存时间构成影响,也无法通过虚引用取得一个对象实例.为一个对象设置虚引用关联的唯一目的就是希望能在这个对象被回收时收到一个系统通知.


 @Test
    public void referenceQueue() throws InterruptedException {
Object strongRef = new Object();
ReferenceQueue<Object> queue = new ReferenceQueue<Object>();
WeakReference<Object> weakRef = new WeakReference<Object>(
strongRef, queue);

assertFalse(weakRef.isEnqueued());
Reference<? extends Object> polled = queue.poll();
assertNull(polled);

strongRef = null;
System.gc();

assertTrue(weakRef.isEnqueued());
Reference<? extends Object> removed = queue.remove();
assertNotNull(removed);
    }



finalize方法.在一个对象在宣告死亡时,至少要经历两次标记过程:如果该对象在进行根搜索后发现没有与GC Roots相连接的引用链,那么他将会被第一次标记并且进行一次筛选,筛选的条件是次对象是否有必要执行finalize方法.当对象没有覆盖finali方法,或者finalize方法已经被虚拟机调用过,虚拟机将这两中情况都视为没有必要执行finalize方法.finalize只会被jvm执行一次。
## CLASS LOADER

 
引导类加载器（bootstrap class loader）：它用来加载 Java 的核心库，负责加载JAVA_HOME\lib目录中并且能被虚拟机识别的类库到JVM内存中，如果名称不符合的类库即使放在lib目录中也不会被加载。该类加载器无法被Java程序直接引用。是用原生代码来实现的，并不继承自 java.lang.ClassLoader。扩展类加载器（extensions class loader）：它用来加载 Java 的扩展库,主要是负责加载JAVA_HOME\lib\ext目录中的类库。Java 虚拟机的实现会提供一个扩展库目录。该类加载器在此目录里面查找并加载 Java 类。该类继承自 java.lang.ClassLoader系统类加载器（system class loader）：它根据 Java 应用的类路径（CLASSPATH）来加载 Java 类。一般来说，Java 应用的类都是由它来完成加载的。可以通过 ClassLoader.getSystemClassLoader()来获取它。该类继承自 java.lang.ClassLoader.双亲委派模型的工作过程为：如果一个类加载器收到了类加载的请求，它首先不会自己去尝试加载这个类，而是把这个请求委派给父类加载器去完成，每一个层次的加载器都是如此，因此所有的类加载请求都会传给顶层的启动类加载器，只有当父加载器反馈自己无法完成该加载请求（该加载器的搜索范围中没有找到对应的类）时，子加载器才会尝试自己去加载。
加载类的过程
在前面介绍类加载器的代理模式的时候，提到过类加载器会首先代理给其它类加载器来尝试加载某个类。这就意味着真正完成类的加载工作的类加载器和启动这个加载过程的类加载器，有可能不是同一个。真正完成类的加载工作是通过调用 defineClass来实现的；而启动类的加载过程是通过调用 loadClass来实现的。前者称为一个类的定义加载器（defining loader），后者称为初始加载器（initiating loader）。在 Java 虚拟机判断两个类是否相同的时候，使用的是类的定义加载器。也就是说，哪个类加载器启动类的加载过程并不重要，重要的是最终定义这个类的加载器。两种类加载器的关联之处在于：一个类的定义加载器是它引用的其它类的初始加载器。如类 com.example.Outer引用了类com.example.Inner，则由类 com.example.Outer的定义加载器负责启动类 com.example.Inner的加载过程。
方法 loadClass()抛出的是 java.lang.ClassNotFoundException异常；方法 defineClass()抛出的是java.lang.NoClassDefFoundError异常。


一般来说，自己开发的类加载器只需要覆写 findClass(String name)方法即可。java.lang.ClassLoader类的方法loadClass()封装了前面提到的代理模式的实现。 


由于加载rt.jar中的类加载器是bootstrap类加载,无法加载到普通的classpath下面的类.所以这个时候,一个workaround的方法就是在调用前设置contextClassloader,然后在加载时,由contextClassloader去加载第三方实现. 

在JDBC4.0驱动实现中,通过jdk1.6的 ServiceLoader机制,可以自动加载第三方驱动,不需要再执行Class.forName(JDBC_DRIVER)了.
 



## CA
超标量设计  颗处理器内核中实行了指令级并行的一类并行运算。这种技术能够在相同的CPU主频下实现更高的CPU吞吐率（throughput）。处理器的内核中一般有多个执行单元（或称功能单元），如算术逻辑单元、位移单元、乘法器等等。未实现超标量体系结构时，CPU在每个时钟周期仅执行单条指令，因此仅有一个执行单元在工作，其它执行单元空闲。超标量体系结构的CPU在一个时钟周期可以同时分派（dispatching）多条指令在不同的执行单元中被执行，这就实现了指令级的并行。超标量体系结构可以视作MIMD（多指令多数据）。


在多核系统中，处理器一般有一层或者多层的缓存，这些的缓存通过加速数据访问（因为数据距离处理器更近）和降低共享内存在总线上的通讯（因为本地缓存能够满足许多内存操作）来提高CPU性能。缓存能够大大提升性能，但是它们也带来了许多挑战。



  1. 请说明下java的内存模型及其工作流程。
  2. 阻塞队列实现一个生产者和消费者模型？请写代码。

  3. COW:读不加锁，写加锁。写时：  final ReentrantLock lock = this.lock; lock.lock();

## 基本面试题 
空指针，数组越界， io，sql classnotfound

###  IO  
 IO                NIO
面向流            面向缓冲阻塞IO            非阻塞IO无                选择器

   
Buffer的四个属性
所有的缓冲区都具有四个属性来提供关于其所包含的数据元素的信息。它们是：

  1. 标记（Mark） 一个备忘位置。调用mark( )来设定mark = postion。调用reset( )设定position = mark。标记在设定前是未定义的(其值为-1)。mark 提供标记,类似书签功能,方便重读.
  2. 位置（Position） 下一个要被读或写的元素的索引。位置会自动由相应的get( )和put( )函数更新。
  3. 上界（Limit） 缓冲区的第一个不能被读或写的元素。或者说，缓冲区中现存元素的计数，指明了缓冲区有效内容的末端。
  4. 容量（Capacity） 缓冲区能够容纳的数据元素的最大数量。这一容量在缓冲区创建时被设定，并且永远不能被改变。
 
  7. 这4个属性大小关系满足： mark <=position <=limit<=capacity
 
NIO.BUFFER
flip()：该方法通常是读取读取后,完成将数据写到buffer中；然后在需要将buffer的数据读出来,供应用程序使用；如果通道现在在缓冲区上执行get()，那么它将从我们刚刚插入的有用数据之外取出未定义数据，所以这个时候就需要先调用flip方法,将limit=position,position=0,mark=-1，这样就可以使程序顺利的从第position个位置开始读数据。其效果基本等同于buffer.limit(buffer.position()).position(0);

 
  1. rewind()：该方法与flip()相似，但不影响limit属性。它只是将position设回0，以便重读缓冲区中的数据。
  2. clear()：方法并没有清除掉原来的buffer中的数据,仅仅是改变属性值：position=0,limit=capacity,mark=-1
 
   1. FileChannel的重要方法 
  2. force(boolean metaData):该方法将数据强制刷新到本次存储设备上,这对确保在系统崩溃时不会丢失重要信息特别有用。metaData 参数可用于限制此方法必需执行的 I/O 操作数量。为此参数传入 false 指示只需将对文件内容的更新写入存储设备；传入 true 则指示必须写入对文件内容和元数据的更新，这通常需要一个以上的 I/O 操作。此参数是否实际有效取决于底层操作系统，因此是未指定的。
  3. map(MapMode, long, long):将此通道的文件区域直接映射到内存中,并返回MappedByteBuffer(通常是DirectByteBuffer)。仅建议在读写相对较大的文件时使用此方法,读写小文件时建议使用read/write方法。可以调用DirectByteBuffer的接口方法clean来及时回收内存。
  4. transferTo(long, long, WritableByteChannel):该方法内部使用了zero-copy机制，避免了不必要的上下文切换开销和缓存区数据复制开销，能够极大的提高传输文件能力。详见参考链接。
 
  1. 传统socket采用PER CONNECTION PER THREAD 来提升性能。但是很多时候，连接建立起来了，但是可能由于客户端一直没输入数据（比如网络聊天等），导致占用内存浪费（但是由于因为每个线程占用1m内存(由-xss参数指定内存大小）。而且，是由于socket 在read inputstream 和 write ouputstream时容易因为网络慢而阻塞,导致系统来回切换线程上下文(切换上下文开销较大)
 
在NIO,由于网络传递数据的速度远远低于CPU的处理数据的速度，并且传递的数据通常放到TCP的协议栈缓存中. 操作系统知道哪些socket的数据包已经准备好了,这样周期性的调用操作系统的Selector.select方法,获取数据准备好的连接。然后针对这些准备好数据的连接,再从中获取SelectionKey的socketChannel,进行处理网络中的数据。
从NIO实现原理来看，操作系统用单个线程来管理哪些连接数据准备好了，并把这些连接维护在一个列表里面，然后客户端程序去遍历这个列表，进行处理。  



## 说明

有部分章节节选自网络，未注明出处。
 
   

inputstreamreader 适配者 bufferinpuntstream 装饰者

面试回答


 活跃用户，redis mget， 
 
ThreadLocal用于创建线程的本地变量，我们知道一个对象的所有线程会共享它的全局变量，所以这些变量不是线程安全的，我们可以使用同步技术。但是当我们不想使用同步的时候，我们可以选择ThreadLocal变量。
 
Lock接口比同步方法和同步块提供了更具扩展性的锁操作。
 
   * 可以使锁更公平
   * 可以使线程在等待锁的时候响应中断
   * 可以让线程尝试获取锁，并在无法获取锁的时候立即返回或者等待一段时间

java.util.concurrent.BlockingQueue的特性是：当队列是空的时，从队列中获取或删除元素的操作将会被阻塞，或者当队列是满时，往队列里添加元素的操作会被阻塞。



FutureTask是Future的一个基础实现，我们可以将它同Executors使用处理异步任务。通常我们不需要使用FutureTask类，单当我们打算重写Future接口的一些方法并保持原来基础的实现是，它就变得非常有用。

 
FileDescriptor.sync() FileChannel.force(boolean metaData) 
 

  

{% include JB/setup %}
