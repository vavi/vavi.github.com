---
layout: post
title: "java thread programing part5 坑和优化"
description: ""
category: 
tags: []
---


那些坑
没指定线程name
没有限定线程个数
没指定默认队列大小
线程池是先指定core size,然后填满队列,最后再到max size.
惊群
stop不安全，其他被废弃的方法
死锁
ThreadLocal没有remove,没有static (线程池情况下)
组合的原子操作并不是线程安全的
惊群现象
不要使用stop等废弃方法 why? 共享变量(需要voliate?) 和中断状态
java client server
9.
  线程饥饿的原因:1.无CPU时间2.等待锁3.处于wait状态,无法被notify.有效避免饥饿的方法:1.使用Lock替换synchronized (这意味着大部分时间用在等待进入锁和进入临界区的过程是用在wait()的等待中，而不是被阻塞在试图进入lock()方法中。？？？),并在finally中调用unlock方法2.使用公平锁(在前者的基础上,内部一般使用队列机制,然后从head中获取需要处理的元素)

     if(!vector.contains(element))
        vector.add(element); 
这个"如果不存在那么添加"操作存在竞态条件,需要额外的锁机制 .       
        
线程非安全(voliate i++(Read-Modify-Write,每一个步骤计算的计算的结果依赖前一个步骤的状态),serverlet,spring 单例,服务无状态)
while(wait)
实现putIfAbsent时,注意list的锁位置,不要使用不同的锁.
CAS的ABA


优化
锁粒度(没必要锁(虽然会优化),范围过大())
cmh的多个锁
伪共享(false sharing)
cas无阻塞,不会导致context switch
没有区分使用原子性,可见性和互斥性场景
通过缩小同步范围,并且并且应该尽量将不影响共享状态的操作从同步代码分离出去.

{% include JB/setup %}
