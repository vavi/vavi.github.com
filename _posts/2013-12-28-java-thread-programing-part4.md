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

java.util.concurrent.locks.Lock接口允许以一种更灵活的方式来构建synchronized块。它比synchronized关键字更加强大，详细如下。

* tryLock，非阻塞性的尝试获取锁，如果获取失败，可以立即进行其他处理。
* tryLock(long time, TimeUnit unit)，在一定时间范围内尝试获取锁，如果在指定时间范围内获取失败，则可以再进行其他处理。
* lockInterruptibly，阻塞性地获取锁，但是允许在阻塞的过程中，被中断。
* Lock接口比synchronized关键字提供更好的性能。

ReentrantLock，CountDownLatch，FutureTask等类都有一个内部类Sync，该Sync类继承了AbstractQueuedSynchronizer(下文简称AQS)。该AQS相当于实现了一个模板模式，把一些独特操作委托给子类实现。这也就是AQS为什么强大的内在原因。

AQS使用了Lock-Free的算法，底层使用了Unsafe类CAS操作改变同步器的state的值，并依据state的值和Node状态来park还是unpark线程。

为了解决锁竞争的问题，采用了FIFO的Node队列。该队列是一个双向链表结构，当锁竞争失败时，把代表线程的Node加到tail节点上；当释放锁时，从head节点移除代表线程的Node。

AQS#state在下文的Mutex类中用来标识资源的可用情况，0标识资源可用，1标识资源被占用，即不可用。

AQS#Node队列是一个CLH队列的变种。waitStatus用来标识线程是否需要被block。当前驱节点释放锁时，则当前节点需要被通知（**sigal**），以便解除阻塞状态，从而进行运行。

Node#waitStatus 这个用来标识节点的状态。在详细介绍之前，先要看下这个类的基本结构以及类的说明。
  
#### 源码分析

```
class Mutex implements Lock, java.io.Serializable {
   // 内部类，自定义同步器
   private static class Sync extends AbstractQueuedSynchronizer {
     
     // 是否处于独占状态,true表示处于独占，false表示非独占
     protected boolean isHeldExclusively() {
       return getState() == 1;
     }
     
     //尝试获取许可资源。在互斥锁里，这个的 acquires 值为1，表示消耗一个资源
     public boolean tryAcquire(int acquires) {
       assert acquires == 1; // Otherwise unused
       if (compareAndSetState(0, 1)) {将state从0设置为1
       	 //当多个线程同时调用tryAcquire时，只有一个线程能够进入下列语句进行执行
         setExclusiveOwnerThread(Thread.currentThread());//设置拥有独占资源的线程
         return true;
       }
       return false;
     }
     // 释放许可资源，在互斥锁里，这个的 releases 值为1，表示释放一个资源
     protected boolean tryRelease(int releases) {
       assert releases == 1; // Otherwise unused
       if (getState() == 0) throw new IllegalMonitorStateException();
       setExclusiveOwnerThread(null);
       setState(0);
       return true;
     }
     // 返回一个Condition，每个condition都包含了一个condition队列。ConditionObject稍后介绍。
     Condition newCondition() { return new ConditionObject(); }
   }
   // 仅需要将操作代理到Sync上即可
   private final Sync sync = new Sync();
   public void lock()                { sync.acquire(1); }
   public boolean tryLock()          { return sync.tryAcquire(1); }
   public void unlock()              { sync.release(1); }
   public Condition newCondition()   { return sync.newCondition(); }
   public boolean isLocked()         { return sync.isHeldExclusively(); }
   public boolean hasQueuedThreads() { return sync.hasQueuedThreads(); }
   public void lockInterruptibly() throws InterruptedException {
     sync.acquireInterruptibly(1);
   }
   public boolean tryLock(long timeout, TimeUnit unit)
       throws InterruptedException {
     return sync.tryAcquireNanos(1, unit.toNanos(timeout));
   }
 }
```

#### 分析Mutex#lock方法

在Mutex的lock方法中，调用sync.acquire(1);如果能够成功获取锁资源，那么继续执行；否则该方法阻塞。
	
```
	public final void acquire(int arg) {
        if (!tryAcquire(arg) &&  //标记1
            acquireQueued(addWaiter(Node.EXCLUSIVE), arg)) //标记2
            selfInterrupt();//标记3
    }
```

tryAcquire 见Mutex类中的代码注释。

在标记1处，tryAcquire(1)如果能够成功获取锁，那么!tryAcquire(1)返回false；此时发生and短路操作，acquire方法成功执行。

在标记1处，tryAcquire(1)如果获取锁失败，那么!tryAcquire(1)返回true；此时继续执行标记2的代码。

在标记2处，addWaiter表示将当前线程代表的节点加到等待队列中去；acquireQueued表示，既然这个节点加入到等待队列后，那么再次尝试获取锁。如果此时还是获取失败，那么我就调用park方法阻塞该线程。下面再详细介绍addWaiter和acquireQueued方法。


```
 	private Node addWaiter(Node mode) {
        Node node = new Node(Thread.currentThread(), mode);//这里mode值为null
        // Try the fast path of enq; backup to full enq on failure
        // 上面英文的意思是，先尝试快速插入到尾节点中，当尝试失败时，可以在enq方法中插入。
        Node pred = tail;
        if (pred != null) {//如果为true，说明该链表不为空；
            node.prev = pred;//在双向FIFO链表中，tail永远指向链表最后一个节点。设置此时node 的前驱结点是tail节点
            if (compareAndSetTail(pred, node)) {//标记4，如果当前tail是pred，那么设置新的尾巴是node节点
                pred.next = node;//设置原来的tail后继结点为node节点
                return node;//至此node成功加入队列中，并且是尾巴节点
            }
        }
        
        //注意上文不是在for循环中执行，而下面的enq方法在for循环中执行，保证最终能将node节点插入链表中
        //如果链表为空，那么执行到enq方法
        enq(node);//node加入到链表中，虽然该方法返回了tail的前驱节点，但是这里并没有使用到enq方法的返回值
        return node;
    }


```

```
	private Node enq(final Node node) {
        for (;;) {
            Node t = tail;
            if (t == null) { // Must initialize，如果此时队列为空
                if (compareAndSetHead(new Node()))//设置head节点
                    tail = head;//设置tail节点，此时head和tail都指向新的node节点，然后进入下一个循环，
            } else {
                node.prev = t;
                if (compareAndSetTail(t, node)) {//和标记4的代码差不多，只不过前者的tail引用命名为pred，这里命名为t，很是干扰人理解。
                    t.next = node;//完成将node节点插入到链表中
                    return t;//注意这里返回原来的tail节点，不是新的尾巴节点，也就是说tail的前驱节点
                }
            }
        }
    }
```


`acquireQueued` 这个方法只有一个`return interrupted;`出口，表示该线程是此执行期间，是否发生中断。

如果`if (p == head && tryAcquire(arg))` 返回false，说明此线程没有获得锁资源，那么该线程需要被park。  
   
`shouldParkAfterFailedAcquire` 表示在线程是否真的需要被park，因为其内部还要取决内`waitStatus`；如果`shouldParkAfterFailedAcquire`返回true，那么`parkAndCheckInterrupt`则真正进行park操作，该线程并会被阻塞。需要注意的是，`parkAndCheckInterrupt`需要在其他线程调用unPark操作时，该线程才会被unBlock。也就是说，通常隔一段时间后，在线程被唤醒时，次方法返回该线程是否中断的表示。
   
如果shouldParkAfterFailedAcquire返回false，那么会继续进入下一个循环。


```
	    final boolean acquireQueued(final Node node, int arg) {
        boolean failed = true;
        try {
            boolean interrupted = false;
            for (;;) {
                final Node p = node.predecessor();
                if (p == head && tryAcquire(arg)) {//标记5，如果当前节点的前驱节点是头结点并且能够成功获取锁。这里需要展开描述下，见代码片段下面的描述
                    setHead(node);//设置头结点是该node节点
                    p.next = null; // help GC
                    failed = false;//这里相当于成功获取获取锁了，就不需要执行后面的cancelAcquire方法
                    return interrupted;
                }
                if (shouldParkAfterFailedAcquire(p, node) &&
                    parkAndCheckInterrupt())
                    interrupted = true;
            }
        } finally {
            if (failed)
                cancelAcquire(node);
        }
    }
```
标记5的条件内涵比较复杂，分为3种情况

*  如果p == head && tryAcquire(arg),那么表示锁资源获取成功
*  如果p!=head，表示node的前驱节点不是head节点，也就是链表中此时已经由多个node
*  如果p == head，但是tryAcquire(arg)=false，表示前驱节点虽然是头结点，但是获取锁资源失败

个人认为下面这段`shouldParkAfterFailedAcquire`代码时最难懂的，因为需要对`waitStatus`有个比较彻底的理解才行。

在互斥模式下，需要先将`pred.waitStatus`改成Node.SIGNAL才行。为什么？这是一种状态约定，同时看到release方法时，它只将状态小于0的线程给unpark了。

这个方法在做3件事情：

* 如果前驱结点的状态是Node.SIGNAL，那么直接返回true；让后面的`parkAndCheckInterrupt`方法block该线程
* 如果前驱结点的状态ws > 0，则表示该结点被取消了，然后循环删除所有前驱结点状态为取消的节点。
* 如果驱结点的状态ws <=0（实际上只能为0 or PROPAGATE），则设置驱结点的状态为Node.SIGNAL

稍微抽象下，如果状态合法，那么直接返回；如果存在无效节点，那么删除；如果这个状态需要被修改，那么修改；然后再循环一次。

也就是说，实际上，如果只有两个线程，第一个线程拥有锁后，第二个线程首先执行`compareAndSetWaitStatus(pred, ws, Node.SIGNAL);`，然后下一次循环时，再执行`if (ws == Node.SIGNAL) return true;`。


```
    private static boolean shouldParkAfterFailedAcquire(Node pred, Node node) {
        int ws = pred.waitStatus;
        if (ws == Node.SIGNAL)
            /*
             * This node has already set status asking a release
             * to signal it, so it can safely park.
             */
            return true;
        if (ws > 0) {
            /*
             * Predecessor was cancelled. Skip over predecessors and
             * indicate retry.
             */
            do {
                node.prev = pred = pred.prev;
            } while (pred.waitStatus > 0);
            pred.next = node;
        } else {
            /*
             * waitStatus must be 0 or PROPAGATE.  Indicate that we
             * need a signal, but don't park yet.  Caller will need to
             * retry to make sure it cannot acquire before parking.
             */
            compareAndSetWaitStatus(pred, ws, Node.SIGNAL);
        }
        return false;
    }
```

`acquireInterruptibly`这方法在等待获取锁时，如果发生中断，则抛出InterruptedException；基本都同`acquire`方法。

`doAcquireNanos(int arg, long nanosTimeout)`这个方法需要说一下的时，如果nanosTimeout小于spinForTimeoutThreshold，则相当于是不会执行park方法；另外这个时间是非精确值，每次计算后，在和当前时间比较。

当线程发生中断或者超时后，会调用`cancelAcquire` 方法（但是我认为普通的acquire方法是不会触发调用`cancelAcquire`方法的，因为其只有一个return出口）。

```
private void cancelAcquire(Node node) {
        // Ignore if node doesn't exist
        if (node == null)
            return;

        node.thread = null;

        // Skip cancelled predecessors
        Node pred = node.prev;
        while (pred.waitStatus > 0)
            node.prev = pred = pred.prev;

        // predNext is the apparent node to unsplice. CASes below will
        // fail if not, in which case, we lost race vs another cancel
        // or signal, so no further action is necessary.
        Node predNext = pred.next;

        // Can use unconditional write instead of CAS here.
        // After this atomic step, other Nodes can skip past us.
        // Before, we are free of interference from other threads.
        node.waitStatus = Node.CANCELLED;//设置取消状态

        // If we are the tail, remove ourselves.
        if (node == tail && compareAndSetTail(node, pred)) {
            compareAndSetNext(pred, predNext, null);
        } else {
            // If successor needs signal, try to set pred's next-link
            // so it will get one. Otherwise wake it up to propagate.
            int ws;
            if (pred != head &&
                ((ws = pred.waitStatus) == Node.SIGNAL ||
                 (ws <= 0 && compareAndSetWaitStatus(pred, ws, Node.SIGNAL))) &&
                pred.thread != null) {
                Node next = node.next;
                if (next != null && next.waitStatus <= 0)
                    compareAndSetNext(pred, predNext, next);
            } else {
                unparkSuccessor(node);
            }

            node.next = node; // help GC
        }
    }
```


#### 分析Mutex#unlock方法

release的方法实现较为简单。

tryRelease的实现同tryAcquire，不用赘述。

```
    public final boolean release(int arg) {
        if (tryRelease(arg)) {
            Node h = head;
            if (h != null && h.waitStatus != 0)
                unparkSuccessor(h);
            return true;
        }
        return false;
    }
```
`if (ws < 0) compareAndSetWaitStatus(node, ws, 0);` 将节点恢复到默认状态，

```
	    private void unparkSuccessor(Node node) {
        /*
         * If status is negative (i.e., possibly needing signal) try
         * to clear in anticipation of signalling.  It is OK if this
         * fails or if status is changed by waiting thread.
         */
        int ws = node.waitStatus;
        if (ws < 0)
            compareAndSetWaitStatus(node, ws, 0);

        /*
         * Thread to unpark is held in successor, which is normally
         * just the next node.  But if cancelled or apparently null,
         * traverse backwards from tail to find the actual
         * non-cancelled successor.
         */
        Node s = node.next;
        if (s == null || s.waitStatus > 0) {
            s = null;
            for (Node t = tail; t != null && t != node; t = t.prev)
                if (t.waitStatus <= 0)
                    s = t;
        }
        if (s != null)
            LockSupport.unpark(s.thread);
    }
```
 
本段落最后，ConditionObject，读写锁以后再分析，给自己再挖一个坑。TODO

---

## 线程Executor框架
### 解决了下面几个问题
* 线程生命周期管理（创建和销毁）开销高
* 无限制创建线程可能导致系统资源不足,报OOM错误
### 功能
* 基于生产者,消费者模式,将任务提交和任务执行的过程分离
* 支持对线程生命周期管理
* 支持收集统计信息
* 支持性能监控机制
* 支持多种线程执行策略
### Executors的主要线程池介绍
  
类接口关系，Executor-->ExecutorService-->AbstractExecutorService-->ThreadPoolExecutor。不得不吐槽下，Executor，ExecutorService命名不足以区分其内涵，我现在依旧傻傻分不清楚。




| 方法名称 | 功能说明 | 使用队列 |
| ------------ | ------------- | ------------ |
| FixedThreadPool | 固定线程池大小  | 无界的LinkedBlockingQueue |
| CachedThreadPool | 可回收空闲线程,最大线程Integer.MAX_VALUE,缓存时间为1min  | SynchronousQueue|
|SingleThreadExecutor|单线程的Executor|无界的LinkedBlockingQueue|
|ScheduledThreadPoolS|延时执行,不需要使用TimerTask.原因是后者1.是基于固定时延而不是固定速率 2.线程执行发生异常时不会自动创建新的线程|DelayedWorkQueue|

底层都是利用ThreadPoolExecutor的



### Callable,Runnable
前者可以返回线程执行的结果FutureTask,后者则不可以.

当你想要取消你已提交给执行者的任务，使用Future接口的cancel()方法。根据cancel()方法参数和任务的状态不同，这个方法的行为将不同：

如果这个任务已经完成或之前的已被取消或由于其他原因不能被取消，那么这个方法将会返回false并且这个任务不会被取消。如果这个任务正在等待执行者获取执行它的线程，那么这个任务将被取消而且不会开始它的执行。

如果这个任务已经正在运行，则视方法的参数情况而定。 cancel()方法接收一个Boolean值参数。如果参数为true并且任务正在运行，那么这个任务将被取消。如果参数为false并且任务正在运行，那么这个任务将不会被取消。

* FutureTask
  *  public V get() throws InterruptedException, ExecutionException
  *  public V get(long timeout, TimeUnit unit)
  *  public boolean cancel(boolean mayInterruptIfRunning) 

### CompletionService ,ExecutorService
* 在批量执行任务的时,前者可以在一旦一个任务完成运行时就可以返回运行结果,然后迭代,最终获得所有任务执行结果;后者则必须等待所有任务完成后才会返回结果.前者响应性更好些.
         
---         

 
## 参考
1. [并发编程网](http://ifeve.com/) 
2. [JAVA并发编程学习笔记之Unsafe类](http://blog.csdn.net/aesop_wubo/article/details/7537278)
3. [源码剖析之sun.misc.Unsafe](http://zeige.iteye.com/blog/1182571)
4. [获取Unsafe类的实例](http://www.cnblogs.com/focusj/archive/2012/02/20/2359119.html)
5. [从Java视角理解系统结构(一)CPU上下文切换](http://ifeve.com/java-context-switch/)
6. [类 LockSupport JAVADOC](http://www.cjsdn.net/Doc/JDK60../java/util/concurrent/locks/LockSupport.html)
7. [用AtomicStampedReference解决ABA问题](http://blog.hesey.net/2011/09/resolve-aba-by-atomicstampedreference.html)
8. [AbstractQueuedSynchronizer的介绍和原理分析](http://ifeve.com/introduce-abstractqueuedsynchronizer/)
9. [初步理解AQS](https://docs.google.com/file/d/0B758ryeeYYQYa1k2cHhzT0ZOSlU/edit?pli=1)
10. [infoq 聊聊并发系列](http://www.infoq.com/cn/author/%E6%96%B9%E8%85%BE%E9%A3%9E)

{% include JB/setup %}
