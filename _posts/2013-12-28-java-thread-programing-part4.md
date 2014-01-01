---
layout: post
title: "JAVA学习笔记--4.多线程编程 part4.JAVA多线程的高级类库"
description: "Java多线程编程"
category: JAVA学习笔记
tags: [JAVA学习笔记]
---
## Unsafe
### 主要功能
#### 获得类，实例属性的offset
* long sun.misc.Unsafe.objectFieldOffset(Field f)
* long sun.misc.Unsafe.staticFieldOffset(Field f)
 
#### 根据对象引用和offset获得相应的值
* int sun.misc.Unsafe.getInt(Object o, long offset)
* void sun.misc.Unsafe.putInt(Object o, long offset, int x)
* Object sun.misc.Unsafe.getObject(Object o, long offset)
* void sun.misc.Unsafe.putObject(Object o, long offset, Object x)
* 其他7种基本类型的读写方法


#### 根据对象引用和offset获得一个拥有volatile语义相应的值
* Object sun.misc.Unsafe.getObjectVolatile(Object o, long offset)
* void sun.misc.Unsafe.putObjectVolatile(Object o, long offset, Object x)
* 8种基本类型的读写方法

#### 管理内存大小
* long sun.misc.Unsafe.allocateMemory(long bytes)
* long sun.misc.Unsafe.reallocateMemory(long address, long bytes)
* long sun.misc.Unsafe.reallocateMemory(long address, long bytes)
* void sun.misc.Unsafe.setMemory(long address, long bytes, byte value)
* void sun.misc.Unsafe.copyMemory(long srcAddress, long destAddress, long bytes)
* void sun.misc.Unsafe.freeMemory(long address)

#### 线程状态管理
* void sun.misc.Unsafe.monitorEnter(Object o)
* void sun.misc.Unsafe.monitorExit(Object o)
* boolean sun.misc.Unsafe.tryMonitorEnter(Object o)
* void sun.misc.Unsafe.unpark(Object thread) 
* void sun.misc.Unsafe.park(boolean isAbsolute, long time)

#### CAS操作
* boolean sun.misc.Unsafe.compareAndSwapObject(Object o, long offset, Object expected, Object x)
* boolean sun.misc.Unsafe.compareAndSwapInt(Object o, long offset, int expected, int x)
* boolean sun.misc.Unsafe.compareAndSwapLong(Object o, long offset, long expected, long x)

#### 读写native pointer
* long sun.misc.Unsafe.getAddress(long address)
* void sun.misc.Unsafe.putAddress(long address, long x)

#### 根据native pointer值读写类，实例属性值
* byte sun.misc.Unsafe.getByte(long address)
* void sun.misc.Unsafe.putByte(long address, byte x)
* 其他7种基本类型的读写方法

### 如何使用
需要在应用程序中反射获取unfafe实例。
	
		private static Unsafe getUnsafeInstance() throws SecurityException,
            NoSuchFieldException, IllegalArgumentException,
            IllegalAccessException {
        Field theUnsafeInstance = Unsafe.class.getDeclaredField("theUnsafe");
        theUnsafeInstance.setAccessible(true);
        return (Unsafe) theUnsafeInstance.get(Unsafe.class);
    	}
### Unsafe小结
顾名思义，这个类是“不安全”。仔细看里面的方法注释，很多都是不校验输入参数的。

由于目前还没有计划研究JVM源码，所以只能浅浅分类列下。

这个类如果要在业务程序中使用，需要使用反射。

通常使用上面方面的两种功能

1. 获取实例的offset，然后调用cas方法，进行原子操作
2. 调用LockSupport的park和unpark方法，对线程进行控制；该方法详见下文介绍LockSupport

---

## LockSupport
该类主要用于管理线程的调度，block或者unblock线程。

* void java.util.concurrent.locks.LockSupport.park(Object blocker)
   
  该方法阻塞当前线程（相当于线程卡在park方法的执行上），直到出现以下3种情况：
    
    1. 该线程被调用unpark方法
    2. 出现中断
    3. 虚假唤醒
    
   该方面和信号量类有一点差异，信号量类在初始化时，一般是存在几个permit的。通常第一次调用acquire方法，是不会被阻塞的。但是park方法不太一样，如果在此之前没有被调用unpark方法，而开始直接调用park方法时，那么该线程会阻塞。

   建议通常使用带blocker参数的方法，并且通常来讲，该blocker的值通常为this（即线程自身）。
   
   还有其他几个带有时间参数的park版本，除了超时参数，以及是绝对时间还是相对时间，其他都差不多。
   
```   
	public static void park(Object blocker) {
        Thread t = Thread.currentThread();
        setBlocker(t, blocker); // 此blocker对象在线程受阻塞时被记录，以允许监视工具和诊断工具确定线程受阻塞的原因。（
        unsafe.park(false, 0L);// false 表示是相对时间，true表示是绝对时间；0L表示永远等待，有点类似wait(0)方法。
        setBlocker(t, null); //? 
    }
```    

* void java.util.concurrent.locks.LockSupport.unpark(Thread thread)

  unblock线程，使permit加1.如果此时线程出于park状态，那么它此时会解除阻塞状态，继续执行。如果此时没有线程处于blocked状态，那么下一次调用park时，该线程不会被阻塞。

```
	public static void unpark(Thread thread) {
        if (thread != null)
            unsafe.unpark(thread);
    }
```    

---

### AbstractQueuedSynchronizer
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
#### 背景知识
##### 链表
#### 概要说明
#### 源码分析
---

## 线程Executor框架
### 解决了下面几个问题
* 线程生命周期管理开销高
* 大量线程消耗CPU和内存资源
* 无限制创建线程可能导致系统资源不足,报OOM错误
### 功能
* 基于生产者,消费者模式,将任务提交和任务执行的过程分离
* 支持对线程生命周期管理
* 支持收集统计信息
* 支持性能监控机制
* 支持多种线程执行策略
### Executors的主要线程池介绍
 
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


| 方法名称 | 功能说明 | 使用队列 |
| ------------ | ------------- | ------------ |
| FixedThreadPool | 固定线程池大小  | 无界的LinkedBlockingQueue |
| CachedThreadPool | 可回收空闲线程,最大线程Integer.MAX_VALUE,缓存时间为1min  | SynchronousQueue|
|SingleThreadExecutor|单线程的Executor|无界的LinkedBlockingQueue|
|ScheduledThreadPoolS|延时执行,不需要使用TimerTask.原因是后者1.是基于固定时延而不是固定速率 2.线程执行发生异常时不会自动创建新的线程|DelayedWorkQueue|

底层都是利用ThreadPoolExecutor的

### Callable,Runnable
* 前者可以返回线程执行的结果FutureTask,后者则不可以.
* FutureTask
  *  public V get() throws InterruptedException, ExecutionException
  *  public V get(long timeout, TimeUnit unit)
  *  public boolean cancel(boolean mayInterruptIfRunning) 

### CompletionService ,ExecutorService
* 在批量执行任务的时,前者可以在一旦一个任务完成运行时就可以返回运行结果,然后迭代,最终获得所有任务执行结果;后者则必须等待所有任务完成后才会返回结果.前者响应性更好些.
         
---         
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
### ThreadLocal （不共享）

ThreadLocal.initialValue

InheritableThreadLocal 类提供线程创建线程的值的遗传性 。如果线程A有一个本地线程变量，然后它创建了另一个线程B，那么线程B将有与A相同的本地线程变量值。 你可以覆盖 childValue() 方法来初始子线程的本地线程变量的值。 它接收父线程的本地线程变量作为参数。
 
## 参考
1. [并发编程网](http://ifeve.com/) 
2. [JAVA并发编程学习笔记之Unsafe类](http://blog.csdn.net/aesop_wubo/article/details/7537278)
3. [源码剖析之sun.misc.Unsafe](http://zeige.iteye.com/blog/1182571)
4. [获取Unsafe类的实例](http://www.cnblogs.com/focusj/archive/2012/02/20/2359119.html)
5. [从Java视角理解系统结构(一)CPU上下文切换](http://ifeve.com/java-context-switch/)
6. [类 LockSupport DOC](http://www.cjsdn.net/Doc/JDK60../java/util/concurrent/locks/LockSupport.html)
7. [初步理解AQS](https://docs.google.com/file/d/0B758ryeeYYQYa1k2cHhzT0ZOSlU/edit?pli=1)
8. [用AtomicStampedReference解决ABA问题](http://blog.hesey.net/2011/09/resolve-aba-by-atomicstampedreference.html)
9. [AbstractQueuedSynchronizer的介绍和原理分析](http://ifeve.com/introduce-abstractqueuedsynchronizer/)
{% include JB/setup %}
