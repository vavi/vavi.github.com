---
layout: post
title: "JAVA学习笔记--4.多线程编程 part3.多线程常见类库源码分析part1.BlockingQueue"
description: "Java多线程编程"
category: JAVA学习笔记
tags: [JAVA学习笔记]
---
JAVA多线程的常见概念简要说明

evernote的bak文件
不变式
临街

# 线程Executor框架
## 解决了下面几个问题
* 线程生命周期管理开销高
* 大量线程消耗CPU和内存资源
* 无限制创建线程可能导致系统资源不足,报OOM错误
# 功能
* 基于生产者,消费者模式,将任务提交和任务执行的过程分离
* 支持对线程生命周期管理
* 支持收集统计信息
* 支持性能监控机制
* 支持多种线程执行策略
# Executors的主要线程池介绍
 


| 方法名称 | 功能说明 | 使用队列 |
| ------------ | ------------- | ------------ |
| FixedThreadPool | 固定线程池大小  | 无界的LinkedBlockingQueue |
| CachedThreadPool | 可回收空闲线程,最大线程Integer.MAX_VALUE,缓存时间为1min  | SynchronousQueue|
|SingleThreadExecutor|单线程的Executor|无界的LinkedBlockingQueue|
|ScheduledThreadPoolS|延时执行,不需要使用TimerTask.原因是后者1.是基于固定时延而不是固定速率 2.线程执行发生异常时不会自动创建新的线程|DelayedWorkQueue|

底层都是利用ThreadPoolExecutor的

# Callable,Runnable
* 前者可以返回线程执行的结果FutureTask,后者则不可以.
* FutureTask
  *  public V get() throws InterruptedException, ExecutionException
  *  public V get(long timeout, TimeUnit unit)
  *  public boolean cancel(boolean mayInterruptIfRunning) 

# CompletionService ,ExecutorService
* 在批量执行任务的时,前者可以在一旦一个任务完成运行时就可以返回运行结果,然后迭代,最终获得所有任务执行结果;后者则必须等待所有任务完成后才会返回结果.前者响应性更好些.
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      
# 取消和关闭
## JAVA中没有一种安全的抢占式方法来停止线程,只有一些协作时机制.
## 一种协作策略
   *  设置”请求取消”标识,周期性的查看.如果设置了该标志,那么任务将提前结束
      * 缺陷是,如果该任务阻塞在一个调用中(比如进行一个长时间的服务调用),那么该任务则不能及时响应任务取消
      
## 使用线程中断
   *  JVM不保证阻塞方法检测到中断的速度,但在实际应用响应速度还是很快的
   *  传递InterruptedException
      * 根本不捕获异常
      * 捕获该异常,简单处理后,再次抛出该异常
   *  恢复中断
      * 由于jvm在抛出IE异常后,会自动将中断状态置为false.所以为了调用栈中更高层的看到这个中断,则需要执行`Thread.currentThread.interrupt()`.
      * 针对interrupt方法,需要注意如下几种情况:
         * 清除中断状态,并抛出IE 
           *  也就方法声明直接抛出IE的:Object的wait(), wait(long), or wait(long, int) 方法, Thread的join(), join(long), join(long, int), sleep(long), 或者 sleep(long, int)   
        * 设置中断状态 
          * 在 interruptible channel进行IO操作时, 该channel会被关闭,然后抛出java.nio.channels.ClosedByInterruptException.
          * 线程阻塞在java.nio.channels.Selector 时,则线程会立即返回一个非0的值.就好像调用了该Selector的wakeup方法.
          * 其他未说明的情况(比如在catch IE后,需要传递中断状态,则调用Thread.currentThread.interrupt(),但是此时不满足上述几种情况,则仅仅是设置线程中断状态,不会抛出IE异常)

# 多线程实现技术
### Lock
### Lock Free
#### CAS

Mutex属于sleep-waiting类型的锁。例如在一个双核的机器上有两个线程(线程A和线程B)，它们分别运行在Core0和Core1上。假设线程A想要通过pthread_mutex_lock操作去得到一个临界区的锁，而此时这个锁正被线程B所持有，那么线程A就会被阻塞(blocking)，Core0 会在此时进行上下文切换(Context Switch)将线程A置于等待队列中，此时Core0就可以运行其他的任务(例如另一个线程C)而不必进行忙等待。而Spin lock则不然，它属于busy-waiting类型的锁，如果线程A是使用pthread_spin_lock操作去请求锁，那么线程A就会一直在 Core0上进行忙等待并不停的进行锁请求，直到得到这个锁为止。

11. 自旋锁:spinlock又称自旋锁，一般要求使用spinlock的临界区尽量简短，这样获取的锁可以尽快释放，以满足其他忙等的线程。Spinlock和mutex不同，spinlock不会导致线程的状态切换(用户态->内核态)，但是spinlock使用不当(如临界区执行时间过长)会导致cpu busy飙高。Spinlock优点：没有昂贵的系统调用，一直处于用户态，执行速度快
Spinlock缺点：一直占用cpu，而且在执行过程中还会锁bus总线，锁总线时其他处理器不能使用总线. Mutex优点：不会忙等，得不到锁会sleep. Mutex缺点：sleep时会陷入到内核态，需要昂贵的系统调用
 
#### STM
### 实例代码
## 多线程类库源码分析
JVM底层代码等下个阶段再去分析。
LockSupport 类似于Linux的lock函数
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

[safe construction ](http://www.ibm.com/developerworks/java/library/j-jtp0618/index.html)
	 
原子服务的组合服务还原子吗

synchronized (new Object()) {}
这段代码其实不会执行任何操作，你的编译器会把它完全移除掉，因为编译器知道没有其他的线程会使用相同的监视器进行同步。要看到其他线程的结果，你必须为一个线程建立happens before关系。

临界区是访问一个共享资源在同一时间不能被超过一个线程执行的代码块。

在多个线程通信中，要注意以下几点：

1. 一旦线程调用了wait()方法，它就释放了所持有的监视器对象上的锁.一旦一个线程被唤醒，不能立刻就退出wait()的方法调用，直到调用notify()的线程退出了它自己的同步块。换句话说：被唤醒的线程必须重新获得监视器对象的锁，才可以退出wait()的方法调用，因为wait方法调用运行在同步块里面。如果多个线程被notifyAll()唤醒，那么在同一时刻将只有一个线程可以退出wait()方法，因为每个线程在退出wait()前必须获得监视器对象的锁。
2. 为了避免信号丢失， 用一个变量来保存是否被通知过。在notify前，设置自己已经被通知过。在wait后，设置自己没有被通知过，需要等待通知。
3. 在wait()/notify()机制中，不要使用全局对象，字符串常量等。应该使用对应唯一的对象。
4. 由于莫名其妙的原因，线程有可能在没有调用过notify()和notifyAll()的情况下醒来。这就是所谓的(虚)假唤醒（spurious wakeups）.

从这个角度来解释 
##### infoq的文章

{% include JB/setup %}
