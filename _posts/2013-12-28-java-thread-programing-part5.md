---
layout: post
title: "java thread programing part5 这些年的那些坑"
description: ""
category: 
tags: [JAVA学习笔记]
---
## 基本要求
* 没指定线程name。
* 没有限定线程个数。
* Thread#stop等被废弃的方法不安全，详见TODO。
* 注意锁作用的对象。１．`synchronized (new Object()) {}`这段代码其实不会执行任何操作，你的编译器会把它完全移除掉，因为编译器知道没有其他的线程会使用相同的监视器进行同步。TODO
* voliate i++是线程非安全，属于Read-Modify-Write操作,每一个步骤计算的计算的结果依赖前一个步骤的状态。
* 注意java命令行的client server 的参数区别，详见TODO。
* 注意锁的力度，尽量减少临界区的大小；通过缩小同步范围,并且并且应该尽量将不影响共享状态的操作从同步代码分离出去。
* 注意组合的原子操作并不是线程安全的。如`if(!vector.contains(element)) 。vector.add(element); `也就是说，"如果不存在那么添加"操作存在竞态条件,需要额外的锁机制。    
* 直接抛出中断异常；捕获异常，处理后再抛出中断异常；设置中断状态，业务程序检查该状态。
* 注意在CAS的ABA问题，主要在链表中危害较大。可以使用AtomicStampedReference来规避。

## 线程通信
* 一旦线程调用了wait()方法，它就释放了所持有的监视器对象上的锁.一旦一个线程被唤醒，不能立刻就退出wait()的方法调用，直到调用notify()的线程退出了它自己的同步块。换句话说：被唤醒的线程必须重新获得监视器对象的锁，才可以退出wait()的方法调用，因为wait方法调用运行在同步块里面。如果多个线程被notifyAll()唤醒，那么在同一时刻将只有一个线程可以退出wait()方法，因为每个线程在退出wait()前必须获得监视器对象的锁。
* 为了避免信号丢失， 用一个变量来保存是否被通知过。在notify前，设置自己已经被通知过。在wait后，设置自己没有被通知过，需要等待通知。
* 在wait()/notify()机制中，不要使用全局对象，字符串常量等。应该使用对应唯一的对象。
* 由于莫名其妙的原因，线程有可能在没有调用过notify()和notifyAll()的情况下醒来。这就是所谓的(虚)假唤醒（spurious wakeups）.
* 注意死锁：死锁是两个或更多线程阻塞着等待其它处于死锁状态的线程所持有的锁。死锁通常发生在多个线程同时但以不同的顺序请求同一组锁的时候。当多个线程需要相同的一些锁，但是按照不同的顺序加锁，死锁就很容易发生。如果能确保所有的线程都是按照相同的顺序获得锁，那么死锁就不会发生。但是需要注意,嵌套管程锁死时锁获取的顺序是一致的.嵌套管程锁死中，线程1持有锁A，同时等待从线程2发来的信号，线程2需要锁A来发信号给线程1。
* 避免饥饿。饥饿的原因：1.无CPU时间2.等待锁3.处于wait状态,无法被notify。
* 注意惊群现象
* 避免虚假唤醒，使用while(condition)

## JUC类库

* 线程池是先指定core size,然后填满队列,最后再到max size。所以要注意参数的合理设置。
* 没指定默认队列大小。
* 在线程池情况下，没有在finally中调用ThreadLocal#remove方法,或者ThreadLocal不是static变量
* 需要注意在使用await()和signal()方法时，如果你在condition上调用await()方法而却没有在这个condition上调用signal()方法，这个线程将永远睡眠下去。


## 设计思路
       
* 尽量使服务无状态，这样可以使用单例，避免多线程并发问题，同时也可以节省CPU和内存的开销。
* 尽量避免伪共享问题，可以通过padding来规避。
 
 
## 参考

[安全构造对象](http://www.ibm.com/developerworks/java/library/j-jtp0618/index.html)
[当构造函数泄露this指针时](http://blog.csdn.net/liuxuejiang158blog/article/details/12891899)  


{% include JB/setup %}
