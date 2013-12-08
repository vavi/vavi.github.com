---
layout: post
title: "Java多线程编程"
description: "Java多线程编程 第二部分"
category: 
tags: [J2SE]
---
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



{% include JB/setup %}
