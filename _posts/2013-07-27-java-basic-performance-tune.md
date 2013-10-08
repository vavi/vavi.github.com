---
layout: post
title: "java basic performance tune"
description: ""
category: 
tags: []
---
调查现状,明确问题的现象
问题解决:
系统架构
代码实现
业务功能,聊天记录,线下分析

如何利用工具分析:
CPU使用率,高 线程状态 

线程
 	过多
 	内存过大
 	锁粒度控制不够
 	死循环(无限递归)
 	HashMap死循环
 	
内存 MAT 未分页,
数据库索引 
表结构设计不合理,没有适当荣誉
paypal架构  

free -m 的误区 

对操作系统的理解

寻找瓶颈点,三个方面验证, 内存,线程和业务日志要互相对应. 
连接超时和缓存

事务不断变化,发展的本质.看完effective java不知道看啥了.

jira宕机后,后台一直抱无法获得新connection错误,直接sqlplus xx/xx@xx 非常缓慢, 经过检查日志,发现后台抱xxx错误,经过google,发现listener日志文件过大, lsnrctl off , start ,然后就恢复了.

性能问题和故障定位

1.启动时死锁,生成多个线程,quartz查询数据   触发加载hibernate的buildSessionFactory,会使用hibernate的annatation的加载,导致hashMap死循环. 解决方法 static holder, 构造方法中加载, 自定义加载hbm.xml , 容器的listener加载顺序.

系统cpu,系统内存,disk io,网卡优化,socket io 目前没遇到过. 各核状态, 高可能是cas,cpu use,但是sys 高 ,可能是线程过多.  jvm 堆内存(各区溢出情况,mat使用技巧(1.history配置打开2.incoming/outcoming 3. shortest gc path 4.hashmap,string 扩容等),), jvm 线程 ,内存 dump 分析.  半夜定时触发gc , jvisualvm btrace

new Object()只需要10条指令了,速度已经超过C(还是C++)了?

{% include JB/setup %}
