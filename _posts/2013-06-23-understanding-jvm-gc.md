---
layout: post
title: "深入理解JVM 内存管理"
description: ""
category: 
tags: [深入理解JVM]
---
### 前言
1. 权利越大,责任越大,对能力要求也越高.在C++中,需要程序员自己来控制内存的申请和释放,从而可以获得更高极限的性能;但是主要有两个方面的原因导致需要自动管理内存的语言:1.在大型系统中,很难避免一些内存管理问题发生;内存管理需要更高技能的程序员,不利于短时间内给公司创造价值.
2. 在解决问题前,首先要充分了解问题,尝试将问题**分成若干个类别**,针对每个类别再去寻找合适的解决方法;在解决问题时,尤其是复杂问题时,我们要学会将复杂问题拆成若干个小问题.

### 数据运行区域
JVM虚拟机在执行java程序的过程中会把其管理的内存划分为若干个不同的数据区域.从这些区域的生命周期来看,有些是**JVM进程**启动时创建,结束时销毁;有的是**线程**启动时创建,结束时销毁.

* **程序计数器**,用来在线程切换后,仍然能够将线程恢复的正确的执行位置.PC的值通常是下一条需要执行的字节码指令.生命周期与线程相同的.
* **Java虚拟机栈**,即我们通常所说的栈,主要用来存放栈帧.栈帧用于存储局部变量表(基本数据类型,对象引用和returnAddress类型)、操作栈、动态链接、方法出口等.生命周期与线程相同的.
* **Java堆**,几乎所有的对象实例和数组都是在堆上分配的(栈上分配,标量替换使得对象实例不一定分配在堆上面).生命周期与JVM 进程相同
* **方法区**,主要用来存放类信息,常量,静态变量,即时编译后的代码等数据.
* **运行时常量池**,该区是方法区的一部分,主要用于存放从编译期可知的数值字面量到必须运行期解析后才能获得的方法或字段引用.Class文件中的常量池在被类加载后就是存放到方法区的运行时常量池中.
* **本地方法栈**,Java虚拟机实现可能会使用到传统的栈（通常称之为“C Stacks”）来支持native方法（指使用Java以外的其他语言编写的方法）的执行，这个栈就是本地方法栈（Native Method Stack）。

### 对象访问
`Ojbect obj = new Object();` 这段简单的语句执行时,牵扯到**栈,堆和方法区**.  HotSpot JVM 中,obj 这个reference直接存放的就是**对象地址**,其目的主要是为了提升性能.

### 垃圾回收
#### 概述 
使用5w2h方法来思考下这个问题

Why:提供内存使用率,进而提供程序性能

When:任何必要的时候(具体的回收算法不同,设置的回收阈值也不同,通常是虚拟机认为内存不够使用的时候)

Where:虚拟机使用的内存区域

Who:虚拟机

What:释放不必要的内存占用

How:1.针对不同的内存区域使用不同的算法;2.设计出更高效率的算法

HowMuch:因使用场景和算法选择而有所不同

#### 垃圾回收策略

**引用计数算法**,该算法主要是:给对象添加一个引用计数器,每当有一个地方引用它时,计数器就加1;当引用失效时,计数器就减1;任何时刻计数器为0的对象就是不可能再被使用的.但是该算法存在如下弊端,它很难解决对象之间互相循环引用的问题.

**根搜索算法**,即 GC Roots Tracing,该算法主要是:以名为"GC Roots"的对象为根节点,从这个根节点向下搜索,所有走过的路径为引用链,当一个对象到GC Roots没有任何引用链相连时,则证明这个对象是不可用的,可以被垃圾回收.主流商用GC语言中,都是选用根搜索算法

**引用类型**,目前主要分为四类.在JDK1.2之前,Java中的引用定义很传统:如果reference类型的数据中存储的数值代表的是另外一块内存的起始地址,就称这块内存代表着一个引用.一个对象在这种定义下,只有被引用和没有被引用两种状态.在很多系统的缓冲功能中,希望一类对象在内存足够的情况下,可以保留在内存之中;如果内存在进行垃圾回收后还是非常紧张,则抛弃这些对象.于是,在JDK1.2之后,Java对引用的概念进行了扩充.

* 强引用,即Strong Reference,这类引用在代码中普遍存在,类似"Object obj = new Object()".只要强引用还存在,垃圾回收就永远不会回收掉被引用的对象
* 软引用,即Soft Reference,用来描述一些还有用,但是非必需的对象.对于软引用关联的对象,在系统将要发生内存溢出异常之前,将会被这些对象列进回收范围内进行第二次垃圾回收.如果这次回收还是没有得到足够的内存,才会抛出内存溢出异常.在JDK1.2之后,提供了SoftReference类来实现软引用
* 弱引用,即Weak Reference,它的强度比软引用还弱一些,只能存活到下一次垃圾回收之前.当垃圾回收时,无论内存是否够用,都会回收掉被弱引用关联的对象.在JDK1.2之后,提供了WeakReference类来实现弱引用.
* 虚引用,即Phantom Reference,它是最弱的一种引用关系.一个对象是否有虚引用的存在,完全不会对其生存时间构成影响,也无法通过虚引用取得一个对象实例.为一个对象设置虚引用关联的唯一目的就是希望能在这个对象被回收时收到一个系统通知.在JDK1.2中,提供了PhantomReference类来实现虚引用.

**finalize方法**.在一个对象在宣告死亡时,至少要经历两次标记过程:如果该对象在进行根搜索后发现没有与GC Roots相连接的引用链,那么他将会被第一次标记并且进行一次筛选,筛选的条件是次对象是否有必要执行finalize方法.当对象没有覆盖finali方法,或者finalize方法已经被虚拟机调用过,虚拟机将这两中情况都视为没有必要执行finalize方法.


####  垃圾回收算法
##### 标记-清除算法
What: 首先标记出需要回收的对象,在标记完成后统一回收掉所有被标记的对象

Pros:实现简单

Cons:1.效率低下 2. 内存碎片
##### 复制算法
What:将可用内存分为2块,每次只使用其中的一块,当这一块的内存用完了,然后将存活的对象复制到另外一块上,然后将已经使用的内存空间一次清理掉.

Pros:1.效率略高 2.没有内存碎片

Cons:1.存在部分内存浪费 2.在对象存活率较高时存在较多内存复制操作
 
##### 标记-整理算法
Waht:在 标记-清除算法基础了,增加了内存整理动作,将所有存活的对象放在一起,然后清除被标记的对象

#####  分代收集算法

堆内存分为**新生代**和老年代,新生代又可以细分为3个,分别是Eden,From Survivor,To Survivor,其内存大小比例默认为8:1:1.

新生代的大部分对象的生命周期都是非常短暂的,所以说,在新生代内存使用时,每次只使用Eden和From Survivor,然后在新生代回收时,大部分对象都可以被回收掉,然后将剩余存活的对象复制到To Survivor中.此时,我们称To Survivor的名称为From Survivor,From Survivor为To Survivor.	

如果在新生代回收时,剩余存活的对象大小超过To Survivor的大小时,我们需要依赖老年代的分配担保(Handle Promotion). 分配担保一般会成功,但是如果此时老年代内存空间也不够了,就会导致分配失败,进而导致Full GC.

所以,一般在新生代中我们选择复制算法,因为新生代每次回收都有大量对象死去,只有少量存活.而老年代则对象成活率较高,没有额外空间对其分配单板,一般使用标记-清理或者标记-整理算法.

#### Hotspot垃圾回收器
![]({{ BASE_PATH }}/images/jvm/jvm-collectors.jpg )	
(FROM https://blogs.oracle.com/jonthecollector/entry/our_collectors)

1.   Serial,单线程收集器,STW,Client模式下得默认新生代收集器,适用于用户的桌面应用场景
2.   ParNew,多线程收集器,STW,可以和CMS搭配使用.
3.   Parallel Scavenge,多线程收集器,STW,吞吐量优先,适合后台运算.
4.   Serial Old,单线程收集器,STW,可以在CMS发生Concurrent Mode Failure 的时候使用
5.   Parallel Old,仅可以和Parallel Scavenge一起配合使用.
6.   CMS(Concurrent Mark Sweep),以获得最短回收停顿时间为目标的收集器,适合B/S系统.它分为4个阶段:初始标记(initial mark),并发标记(concurrent mark),重新标记(remark),并发清除(concurrent sweep).其中,初始标记和重新标记仍然是STW.它主要由3个缺点:1.对CPU资源敏感2.存在内存碎片3.存在浮动垃圾(Floating Garbage,意思是在CMS并发清理阶段用户线程还在运行,还会有新的垃圾产生,这些垃圾只能在下一次GC时再清理掉.)
7.   G1,todo.
8.   下面摘自jonthecollector blog,

* "Serial" is a stop-the-world, copying collector which uses a single GC thread.
* "ParNew" is a stop-the-world, copying collector which uses multiple GC threads. It differs from "Parallel Scavenge" in that it has enhancements that make it usable with CMS. For example, "ParNew" does the synchronization needed so that it can run during the concurrent phases of CMS.
* "Parallel Scavenge" is a stop-the-world, copying collector which uses multiple GC threads.
* "Serial Old" is a stop-the-world, mark-sweep-compact collector that uses a single GC thread.
* "CMS" is a mostly concurrent, low-pause collector.
* "Parallel Old" is a compacting collector that uses multiple GC threads.

#### 常用GC启动参数
该章节见另一篇[JVM启动参数](/2013/07/07/java-jvm-option/) 



UseSerialGC is "Serial" + "Serial Old"
UseParNewGC is "ParNew" + "Serial Old"
UseConcMarkSweepGC is "ParNew" + "CMS" + "Serial Old". "CMS" is used most of the time to collect the tenured generation. "Serial Old" is used when a concurrent mode failure occurs.
UseParallelGC is "Parallel Scavenge" + "Serial Old"
UseParallelOldGC is "Parallel Scavenge" + "Parallel Old"

Fi
rst Header | Second Header | Third Header
------------ | ------------- | ------------
Content Cell | Content Cell  | Content Cell
Content Cell | Content Cell  | Content Cell

### 参考
http://docs.oracle.com/javase/specs/jvms/se7/html/jvms-2.html#jvms-2.5

深入理解Java虚拟机,周志明著

https://blogs.oracle.com/jonthecollector/entry/our_collectors

{% include JB/setup %}
