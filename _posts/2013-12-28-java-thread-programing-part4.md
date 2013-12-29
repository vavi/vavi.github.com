---
layout: post
title: "JAVA学习笔记--4.多线程编程 part4.JAVA多线程的高级类库"
description: "Java多线程编程"
category: JAVA学习笔记
tags: [JAVA学习笔记]
---
unsafe
locksupprot
aqs
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


而锁还有读写锁,公平锁,非公平锁等等
 
## 参考
1. [并发编程网](http://ifeve.com/) 
{% include JB/setup %}
