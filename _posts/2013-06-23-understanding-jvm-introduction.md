---
layout: post
title: "JAVA学习笔记--0.概述"
description: ""
category: 
tags: [JAVA学习笔记]
---
### JVM,JRE,JDK区别
JDK包含JRE,JRE包含JVM.详细介绍见  [all the component technologies in Java SE platform ](http://docs.oracle.com/javase/6/docs/ )

====
### JDK命名规则
* 面向开发者,使用       JDK 1.5,JDK 1.6
* 面向商业宣传,使用     JDK 5,JDK 6

====
### HOTSPOT由来
顾名思义,即为"热点",意思是在Hotspot JVM中,通过JIT技术将JAVA的热点代码编译成本地代码,提高运行效率

====
### 为什么要编译自己的JDK
1. 在实际调试中,由于ORACLE/SUN公司没有release一个debugable rt.jar,所以在debug 一些JDK自带类库时(比如HashMap),是无法看到HashMap里面的变量值的. 而不明真相的观众,往往会以为这是个Eclipse的bug .传送门:  [debugging jdk code ](http://www.eclipse.org/forums/index.php/t/67180/ ). 
2. 出于研究JVM的内部实现目的的话,还是需要自己动手编译一下JDK


{% include JB/setup %}
