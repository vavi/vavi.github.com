---
layout: post
title: "JAVA学习笔记--4.多线程编程 part1.背景知识和内存模型"
description: "Java多线程编程"
category: JAVA学习笔记
tags: [JAVA学习笔记]
---


## 背景知识
### CPU Cache
 ![CPU Cache 示意图]({{ BASE_PATH }}/images/java_note/thread/cpu_cache.png )

如上简易图所示，在当前主流的CPU中，每个CPU有多个核组成的。每个核拥有自己的寄存器，L1,L2 Cache。
多个核之间共享L3 Cache。内存的数据也被多个核共享。在多核系统中，这些的缓存通过加速数据访问和降低共享内存在总线上的通讯来提高CPU性能。

比较经典的Cache一致性协议当属MESI协议，很多处理器都是使用它或者它的的变种。M表示Modified，E表示Exclusive，S表示Shared，I表示Invalid。详见[《大话处理器》Cache一致性协议之MESI][MESI]
 
  [MESI]: http://blog.csdn.net/muxiqingyang/article/details/6615199  "《大话处理器》Cache一致性协议之MESI"
### CPU乱序执行(CPU Out-of-order Execution)
现在的CPU一般采用流水线来执行指令. 一个指令的执行被分成: 取指, 译码, 访存, 执行,写回, 等若干个阶段. 

当CPU在执行指令时，如果发现所需要的数据不在Cache时，则需要从外部存储器中取，而这个过程通常需要几十，甚至几百个Cycle。如果CPU是顺序执行这些指令时，则后面的指令需要等待。所以如果CPU支持乱序执行的话，那么就可以先执行后面不依赖该数据的指令，进而提升CPU计算性能。

### 编译器重排序优化（Compiler Reordering Optimizations）
由于CPU流水线的指令预取范围有限，所以只能在很小的范围内判断指令是否能够并发执行。编译器可以分析相当长的一段代码，从而把能够并发执行的代码尽量靠近，这就是所谓的编译器优化。
 
### 内存屏障
如上面所说,对于” a++；b = f(a)；c = f”等存在依赖关系的指令，CPU则会在“b= f(a)”执行阶段之前被阻塞；另一方面，编译器也有可能将依赖关系很近“人为地”拉开距离以防止阻塞情况的发生，从而导致编译器乱序，如“a++ ；c = f；b = f(a)”。 

没有关联的内存操作会被按随机顺序有效的得到执行， 但是在CPU与CPU交互时或CPU与IO设备交互时，这可能会成为问题。我们需要一些手段来干预编译器和CPU, 使其限制指令顺序。

内存屏障能保证处于内存屏障两边的内存操作满足部分有序。这里"部分有序"的意思是，内存屏障之前的操作都会先于屏障之后的操作，但是如果几个操作出现在屏障的同一边，则不保证它们的顺序。

内存屏障分为编译器内存屏障和CPU内存屏障。

在GCC中，内存屏障是`asm volatile("" ::: "memory");`或者`__asm__ __volatile__ ("" ::: "memory");`；

在Linux中，内存屏障根据 UP还是SMP而有所不同。详见[LINUX内核之内存屏障][MB]
 
内存屏障是否保证CPU将CPU缓存刷写回内存？我在@何_登成大牛的[blog](http://hedengcheng.com/?p=803#comment-5465) 上面提问了下，但是没有得到明确地回复。

 [MB]: http://www.cnblogs.com/icanth/archive/2012/06/10/2544300.html  "LINUX内核之内存屏障"

### 锁（信号）

在所有的 X86 CPU 上都具有锁定一个特定内存地址的能力，当这个特定内存地址被锁定后，它就可以阻止其他的系统总线读取或修改这个内存地址。这种能力是通过 LOCK 指令前缀再加上下面的汇编指令来实现的。当使用 LOCK 指令前缀时，它会使 CPU 宣告一个 LOCK# 信号，这样就能确保在多处理器系统或多线程竞争的环境下互斥地使用这个内存地址。当指令执行完毕，这个锁定动作也就会消失。

在一些处理器，包括 P6 家族，奔腾4(Pentium4)系列，至强(Xeon)处理器，lock 操作可能不会宣告一个 LOCK# 信号。从 P6 家族处理器开始，当使用 LOCK 指令访问的内存已经被处理器加载到缓存中时，LOCK# 信号通常不会被宣告。取而代之的是，仅是锁定了处理器的缓存。这里，处理器缓存的相干性(coherency)机制确保了可以原子性的对内存进行操作。

这个lock 仅仅是锁定单条汇编语言的执行，并不是范围锁。至于范围锁的具体实现，还是需要看下Linux底层的pthread_mutex_lock的源码。但是目前Linux底层功力还很浅薄。。
## JMM
在多线程环境下，由于CPU是多核的，各个核之间的数据修改对方可能感受不到，所以会产生**可见性问题**；
由于上文提到的编译器的重排序优化（编译器）和CPU乱序执行（执行期），产生**无序性问题**。

JMM为了解决如下三个相互牵连的问题：

* 原子性：哪些指令必须是不可分割的。在Java内存模型中，这些规则需声明仅适用于-—实例变量和静态变量，也包括数组元素，但不包括方法中的局部变量-—的内存单元的简单读写操作。
* 可见性：在哪些情况下，一个线程执行的结果对另一个线程是可见的。这里需要关心的结果有，写入的字段以及读取这个字段所看到的值。
* 有序性：在什么情况下，某个线程的操作结果对其它线程来看是无序的。最主要的乱序执行问题主要表现在读写操作和赋值语句的相互执行顺序上。

详细介绍见[深入理解Java内存模型 系列文章](http://www.infoq.com/cn/articles/java-memory-model-1)，下文仅是用通俗的文字，概略介绍下。
    
### 原子性
无论是数据库的ACID还是其他什么模型，谈到原子必然是指一个操作的结果要么是成功，要么是失败，不允许存在中间的状态。在C语言的多线程环境下，如果两个线程同时写一个内存地址，则可能出现混乱的数据。所以JMM约定了在JAVA中，哪些数据的操作是原子的，防止产生混乱的数据。
 
### 可见性
由于CPU的寄存器是不共享的，而内存才是共享的。所以一个CORE的对数据的修改，无法立即让另外的CORE感知（MESI仅是维护缓存一致性，而不是内存一致性）。

所以,JVM以及底层系统提供了大概3种实现机制来保证可见性:

* spinlock 和 mutex 都能保证原子性，可见性和下个章节的介绍的有序性
* volatile能够保证可见性和下个章节的介绍的有序性，能够保证对volatile的单个操作的原子性，但是volatile var ++这种操作时非原子的。

究其本质实现，都是指令执行完成后，强制将所有写过的变量刷新到内存中区。其他线程取值时，需要重新从内存中加载。

### 有序性

  从某个线程的角度看方法的执行，指令会按照一种叫“串行”（as-if-serial）的方式执行，此种方式已经应用于顺序编程语言。
  
 这个线程“观察”到其他线程并发地执行非同步的代码时，任何代码都有可能交叉执行。唯一起作用的约束是：对于同步方法，同步块以及volatile字段的操作仍维持相对有序。
 
#### 通俗理解happen-before规则
	与程序员密切相关的happens-before规则如下：

	1.程序顺序规则：一个线程中的每个操作，happens- before 于该线程中的任意后续操作。
	2.监视器锁规则：对一个监视器锁的释放，happens- before 于随后对这个监视器锁的获取。
	3.volatile变量规则：对一个volatile域的写，happens- before 于任意后续对这个volatile域的读。
	4.传递性：如果A happens- before B，且B happens- before C，那么A happens- before C。
	
第一条规则针对线程内的代码执行。后面3条规则都是线程间的。

第二条针对锁，强调一个线程监视器锁的释放必然早于另一个线程监视器锁的获得，因为这个锁只能同时被一个线程使用。

第三题条针对volatile，写早于读，这样就能及时最新的数据，否则一直读旧数据，肯定是有问题的。

第四条纯粹是数学逻辑游戏。

## 后记
理解多线程实现原理需要大量的知识，需要从硬件的CPU体系结构，汇编原理，Linux的库函数...，很是痛苦。虽然收获也比较大，但是感觉还是没有彻底串起来。需要进一步加油。


## 参考
[Memory Barrier in Compiler and CPU]( http://www.cnblogs.com/whyandinside/archive/2012/06/24/2560099.html
)

[LINUX内核之内存屏障简介]( http://www.cnblogs.com/icanth/archive/2012/06/10/2544300.html
)

[Cache Concurrency Problem - False Sharing]( http://www.cnblogs.com/polymorphism/archive/2011/12/08/2281151.html
)

[LINUX内核内存屏障 LINUX官方文档翻译]( http://blog.csdn.net/ctthuangcheng/article/details/8916084)

[LINUX内核内存屏障 LINUX官方文档英文版](https://www.kernel.org/doc/Documentation/memory-barriers.txt)

[WIKI的MESI_protocol](http://en.wikipedia.org/wiki/MESI_protocol)

[INFOQ的深入理解Java内存模型 系列文章](http://www.infoq.com/cn/articles/java-memory-model-1)

[何登成的技术博客 CPU Cache and Memory Ordering——并发程序设计入门](http://hedengcheng.com/?p=648)


{% include JB/setup %}
