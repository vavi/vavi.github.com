---
layout: post
title: "JAVA学习笔记--4.多线程编程 part2.多线程关键字"
description: "Java多线程编程"
category: JAVA学习笔记
tags: [JAVA学习笔记]
---
## 多线程关键字
### synchronized
首先从我们的日常生活讲起。假设一个教室由N个座位，每个同学只能占一个座位。当N+M个同学来占座时，则M个同学需要等待。座位即资源，同学即线程。占座可以理解为占用资源，在代码中常用lock（acquire）表示，如果没有座位，我就在教室外面**等待**，出于阻塞状态（blocked）；离开座位可以理解为释放所占用的资源，在代码中常用unlock（release）表示。当某同学离开座位时，需要该同学**发出一个信号**，通知外面等待的同学进来占座。

上面的例子实际上对应的是JAVA里面的Semaphore类；而synchronized则是Semaphore的一个特例，即上面的座位数N=1。 synchronized 表达的语义表达的有3个：1.互斥，即同时只有一个线程能够获取锁 2.内存可见，即离开同步块后，临界区内的操作都已经刷新到内存里面 3.有序：同步块里面的代码块内部执行可以乱序，但是必须保证进入同步块和离开同步块的代码不能被乱序执行。

从锁的实现原理来看，底层必须存在某个共享变量，所有线程在争夺锁资源时，必须首先查看该共享变量的状态。
基于这种前提，就能够很好解释为什么使用synchronized（lock）这个block块了。JVM在实现锁时，把锁的状态放在Java对象的头部了 TODO。

在多个线程通信中，要注意以下几点：

1. 一旦线程调用了wait()方法，它就释放了所持有的监视器对象上的锁.一旦一个线程被唤醒，不能立刻就退出wait()的方法调用，直到调用notify()的线程退出了它自己的同步块。换句话说：被唤醒的线程必须重新获得监视器对象的锁，才可以退出wait()的方法调用，因为wait方法调用运行在同步块里面。如果多个线程被notifyAll()唤醒，那么在同一时刻将只有一个线程可以退出wait()方法，因为每个线程在退出wait()前必须获得监视器对象的锁。
2. 为了避免信号丢失， 用一个变量来保存是否被通知过。在notify前，设置自己已经被通知过。在wait后，设置自己没有被通知过，需要等待通知。
3. 在wait()/notify()机制中，不要使用全局对象，字符串常量等。应该使用对应唯一的对象。
4. 由于莫名其妙的原因，线程有可能在没有调用过notify()和notifyAll()的情况下醒来。这就是所谓的(虚)假唤醒（spurious wakeups）.


### voliatile

与使用synchronized相比，声明一个volatile字段的区别在于没有涉及到锁操作。但特别的是对volatile字段进行“++”这样的读写操作不会被当做原子操作执行。


从原子性，可见性和有序性的角度分析，声明为volatile字段的作用相当于一个类通过get/set同步方法保护普通字段，如下：
    
     final class VFloat {
         private float value;

             final synchronized void set(float f)
             { value = f; }
             
             final synchronized float get()      
         { return value; }
     }

另外，有序性和可见性仅对volatile字段进行一次读取或更新操作起作用。声明一个引用变量为volatile，不能保证通过该引用变量访问到的非volatile变量的可见性。同理，声明一个数组变量为volatile不能确保数组内元素的可见性。volatile的特性不能在数组内传递，因为数组
里的元素不能被声明为volatile。


由于没有涉及到锁操作，声明volatile字段很可能比使用同步的开销更低，至少不会更高。但如果在方法内频繁访问volatile字段，很可能导致更低的性能，这时还不如锁住整个方法。

如果你不需要锁，把字段声明为volatile是不错的选择，但仍需要确保多线程对该字段的正确访问。可以使用volatile的情况包括：

该字段不遵循其他字段的不变式。

对字段的写操作不依赖于当前值。

没有线程违反预期的语义写入非法值。

读取操作不依赖于其它非volatile字段的值。

当只有一个线程可以修改字段的值，其它线程可以随时读取，那么把字段声明为volatile是合理的。例如，一个名叫Thermometer(中文：体温计)的类，可以声明temperature字段为volatile。正如在3.4.2节所讨论，一个volatile字段很适合作为完成某些工作的标志。另一个例子在4.4节有描述，通过使用轻量级的执行框架使某些同步工作自动化，但是仍需把结果字段声明为volatile，使其对各个任务都是可见的。

Volitile字段是用于线程间通讯的特殊字段。每次读volitile字段都会看到其它线程写入该字段的最新值；实际上，程序员之所以要定义volitile字段是因为在某些情况下由于缓存和重排序所看到的陈旧的变量值是不可接受的。编译器和运行时禁止在寄存器里面分配它们。它们还必须保证，在它们写好之后，它们被从缓冲区刷新到主存中，因此，它们立即能够对其他线程可见。相同地，在读取一个volatile字段之前，缓冲区必须失效，因为值是存在于主存中而不是本地处理器缓冲区。在重排序访问volatile变量的时候还有其他的限制。

在旧的内存模型下，访问volatile变量不能被重排序，但是，它们可能和访问非volatile变量一起被重排序。这破坏了volatile字段从一个线程到另外一个线程作为一个信号条件的手段。

在新的内存模型下，volatile变量仍然不能彼此重排序。和旧模型不同的时候，volatile周围的普通字段的也不再能够随便的重排序了。写入一个volatile字段和释放监视器有相同的内存影响，而且读取volatile字段和获取监视器也有相同的内存影响。事实上，因为新的内存模型在重排序volatile字段访问上面和其他字段（volatile或者非volatile）访问上面有了更严格的约束。当线程A写入一个volatile字段f的时候，如果线程B读取f的话 ，那么对线程A可见的任何东西都变得对线程B可见了。

如下例子展示了volatile字段应该如何使用：

     class VolatileExample {
       int x = 0;
       volatile boolean v = false;
       public void writer() {
         x = 42;
    v = true;
       }

       public void reader() {
         if (v == true) {
           //uses x - guaranteed to see 42.
         }
       }
     }
    
假设一个线程叫做“writer”，另外一个线程叫做“reader”。对变量v的写操作会等到变量x写入到内存之后，然后读线程就可以看见v的值。因此，如果reader线程看到了v的值为true，那么，它也保证能够看到在之前发生的写入42这个操作。而这在旧的内存模型中却未必是这样的。如果v不是volatile变量，那么，编译器可以在writer线程中重排序写入操作，那么reader线程中的读取x变量的操作可能会看到0。

实际上，volatile的语义已经被加强了，已经快达到同步的级别了。为了可见性的原因，每次读取和写入一个volatile字段已经像一个半同步操作了


  * 把变量声明为volatile类型后,编译器和运行时都会注意到这个变量是共享的,因此**不会将该变量上的操作和其他内存操作进行重排序**.volatile变量不会被缓存在寄存器或者处理器其他不可见的低分,因此在读取volatile类型的变量总是会返回最新写入的值
     * 对volatile的变量读写操作可以分别理解为synchronized get,set方法.但是区别是读写volatile变量不会执行加锁操作.所以说,volatile变量是一种比synchronized关键字更轻量级的一种同步.
     * volatile 变量通常用作表示某个操作完成,发生中断或者状态的标志;对volatile变量的写入操作应该不依赖当前volatile变量的值.

何登成blog c区别
v++；
### final
## 多线程实现技术
### Lock
### Lock Free
#### CAS

Mutex属于sleep-waiting类型的锁。例如在一个双核的机器上有两个线程(线程A和线程B)，它们分别运行在Core0和Core1上。假设线程A想要通过pthread_mutex_lock操作去得到一个临界区的锁，而此时这个锁正被线程B所持有，那么线程A就会被阻塞(blocking)，Core0 会在此时进行上下文切换(Context Switch)将线程A置于等待队列中，此时Core0就可以运行其他的任务(例如另一个线程C)而不必进行忙等待。而Spin lock则不然，它属于busy-waiting类型的锁，如果线程A是使用pthread_spin_lock操作去请求锁，那么线程A就会一直在 Core0上进行忙等待并不停的进行锁请求，直到得到这个锁为止。

11. 自旋锁:spinlock又称自旋锁，一般要求使用spinlock的临界区尽量简短，这样获取的锁可以尽快释放，以满足其他忙等的线程。Spinlock和mutex不同，spinlock不会导致线程的状态切换(用户态->内核态)，但是spinlock使用不当(如临界区执行时间过长)会导致cpu busy飙高。Spinlock优点：没有昂贵的系统调用，一直处于用户态，执行速度快
Spinlock缺点：一直占用cpu，而且在执行过程中还会锁bus总线，锁总线时其他处理器不能使用总线. Mutex优点：不会忙等，得不到锁会sleep. Mutex缺点：sleep时会陷入到内核态，需要昂贵的系统调用
 
#### STM
### 实例代码
## 多线程类库源码分析
InteruptedException
HashTable
HashMap
Collections.synchronized
ConncurrentHashMap
RING BUFFER
### BlockingQueue

阻塞集合：这种集合包括添加和删除数据的操作。如果操作不能立即进行，是因为集合已满或者为空，该程序将被阻塞，直到操作可以进行。
非阻塞集合：这种集合也包括添加和删除数据的操作。如果操作不能立即进行，这个操作将返回null值或抛出异常，但该线程将不会阻塞。

阻塞 异步  http://www.zhihu.com/question/19732473
iteye帖子
http://www.ibm.com/developerworks/cn/linux/l-async/

Unfafe
LockSupport


### 线程执行者
Executor-->ExecutorService-->AbstractExecutorService-->ThreadPoolExecutor
Executors 主要是ThreadPoolExecutor和
几种方法差别,线程池大小和队列不同.

当你想要取消你已提交给执行者的任务，使用Future接口的cancel()方法。根据cancel()方法参数和任务的状态不同，这个方法的行为将不同：

如果这个任务已经完成或之前的已被取消或由于其他原因不能被取消，那么这个方法将会返回false并且这个任务不会被取消。
如果这个任务正在等待执行者获取执行它的线程，那么这个任务将被取消而且不会开始它的执行。如果这个任务已经正在运行，则视方法的参数情况而定。 cancel()方法接收一个Boolean值参数。如果参数为true并且任务正在运行，那么这个任务将被取消。如果参数为false并且任务正在运行，那么这个任务将不会被取消。

如果你想要等待一个任务完成，你可以使用以下两种方法：

如果任务执行完成，Future接口的isDone()方法将返回true。
ThreadPoolExecutor类的awaitTermination()方法使线程进入睡眠，直到每一个任务调用shutdown()方法之后完成执行。
这两种方法都有一些缺点。第一个方法，你只能控制一个任务的完成。第二个方法，你必须等待一个线程来关闭执行者，否则这个方法的调用立即返回。

### ABS


它是基于Lock接口和实现它的类（如ReentrantLock）。这种机制有如下优势：

它允许以一种更灵活的方式来构建synchronized块。使用synchronized关键字，你必须以结构化方式得到释放synchronized代码块的控制权。Lock接口允许你获得更复杂的结构来实现你的临界区。
Lock 接口比synchronized关键字提供更多额外的功能。新功能之一是实现的tryLock()方法。这种方法试图获取锁的控制权并且如果它不能获取该锁，是因为其他线程在使用这个锁，它将返回这个锁。使用synchronized关键字，当线程A试图执行synchronized代码块，如果线程B正在执行它，那么线程A将阻塞直到线程B执行完synchronized代码块。使用锁，你可以执行tryLock()方法，这个方法返回一个 Boolean值表示，是否有其他线程正在运行这个锁所保护的代码。
当有多个读者和一个写者时，Lock接口允许读写操作分离。
Lock接口比synchronized关键字提供更好的性能。

一个锁可能伴随着多个条件。这些条件声明在Condition接口中。 这些条件的目的是允许线程拥有锁的控制并且检查条件是否为true，如果是false，那么线程将被阻塞，直到其他线程唤醒它们。Condition接口提供一种机制，阻塞一个线程和唤醒一个被阻塞的线程。

所 有Condition对象都与锁有关，并且使用声明在Lock接口中的newCondition()方法来创建。使用condition做任何操作之前， 你必须获取与这个condition相关的锁的控制。所以，condition的操作一定是在以调用Lock对象的lock()方法为开头，以调用相同 Lock对象的unlock()方法为结尾的代码块中。

当一个线程在一个condition上调用await()方法时，它将自动释放锁的控制，所以其他线程可以获取这个锁的控制并开始执行相同操作，或者由同个锁保护的其他临界区。

注释：当一个线程在一个condition上调用signal()或signallAll()方法，一个或者全部在这个condition上等待的线程将被唤醒。这并不能保证的使它们现在睡眠的条件现在是true，所以你必须在while循环内部调用await()方法。你不能离开这个循环，直到 condition为true。当condition为false，你必须再次调用 await()方法。

你必须十分小心 ，在使用await()和signal()方法时。如果你在condition上调用await()方法而却没有在这个condition上调用signal()方法，这个线程将永远睡眠下去。

在调用await()方法后，一个线程可以被中断的，所以当它正在睡眠时，你必须处理InterruptedException异常。

### ThreadLocal （不共享）


ThreadLocal.initialValue

InheritableThreadLocal 类提供线程创建线程的值的遗传性 。如果线程A有一个本地线程变量，然后它创建了另一个线程B，那么线程B将有与A相同的本地线程变量值。 你可以覆盖 childValue() 方法来初始子线程的本地线程变量的值。 它接收父线程的本地线程变量作为参数。

## 多线程的坑
5. 死锁是两个或更多线程阻塞着等待其它处于死锁状态的线程所持有的锁。死锁通常发生在多个线程同时但以不同的顺序请求同一组锁的时候。当多个线程需要相同的一些锁，但是按照不同的顺序加锁，死锁就很容易发生。
如果能确保所有的线程都是按照相同的顺序获得锁，那么死锁就不会发生。但是需要注意,嵌套管程锁死时锁获取的顺序是一致的.嵌套管程锁死中，线程1持有锁A，同时等待从线程2发来的信号，线程2需要锁A来发信号给线程1。
  	
False Sharing
多线程要解决问题的本质：如何保证一个线程对对一个共享的资源的操作的结果，能够正确的反应到另一个线程对这个这个共享资源进行操作上来。

线程间通信
	 
原子服务的组合服务还原子吗

synchronized (new Object()) {}
这段代码其实不会执行任何操作，你的编译器会把它完全移除掉，因为编译器知道没有其他的线程会使用相同的监视器进行同步。要看到其他线程的结果，你必须为一个线程建立happens before关系。

临界区是访问一个共享资源在同一时间不能被超过一个线程执行的代码块。


从这个角度来解释 
##### infoq的文章


{% include JB/setup %}
