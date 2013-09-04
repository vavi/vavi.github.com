---
layout: post
title: "java something about classloader"
description: ""
category: 
tags: []
---
关于Java的CLassLoader,这篇文章 [深入探讨 Java 类加载器](http://www.ibm.com/developerworks/cn/java/j-lo-classloader/) 写得很不错,我就不再费笔墨重写这些内容了.

本文主要是笔者对其内容的一些理解和一些未在上文中提到的内容.

好吧,先来炒冷饭:引导类加载器,扩展类加载器,系统类加载器(也叫应用类加载器) 的关系

	引导类加载器（bootstrap class loader）：它用来加载 Java 的核心库，负责加载JAVA_HOME\lib目录中并且能被虚拟机识别的类库到JVM内存中，如果名称不符合的类库即使放在lib目录中也不会被加载。该类加载器无法被Java程序直接引用。是用原生代码来实现的，并不继承自 java.lang.ClassLoader。

	扩展类加载器（extensions class loader）：它用来加载 Java 的扩展库,主要是负责加载JAVA_HOME\lib\ext目录中的类库。Java 虚拟机的实现会提供一个扩展库目录。该类加载器在此目录里面查找并加载 Java 类。该类继承自 java.lang.ClassLoader

	系统类加载器（system class loader）：它根据 Java 应用的类路径（CLASSPATH）来加载 Java 类。一般来说，Java 应用的类都是由它来完成加载的。可以通过 ClassLoader.getSystemClassLoader()来获取它。该类继承自 java.lang.ClassLoader.
	
	双亲委派模型的工作过程为：如果一个类加载器收到了类加载的请求，它首先不会自己去尝试加载这个类，而是把这个请求委派给父类加载器去完成，每一个层次的加载器都是如此，因此所有的类加载请求都会传给顶层的启动类加载器，只有当父加载器反馈自己无法完成该加载请求（该加载器的搜索范围中没有找到对应的类）时，子加载器才会尝试自己去加载。
	
loadClass 实现默认是双亲委派,如果仍然找不到,会调用findClass,默认得findClass会抛出异常.所以一般自定义ClassLoader时,要覆写findClass,然后findClass中调用defineClass.在自己测试自定义classloader的时候,注意不要需要加载的class放到app classloader的 classpath下面,不然还是会被app classloader加载到.

Thread的currentContextLoader默认和父线程的currentContextLoader相同.Java 提供了很多服务提供者接口（Service Provider Interface，SPI），允许第三方为这些接口提供实现。常见的 SPI 有 JDBC、JCE、JNDI、JAXP 和 JBI 等。这些SPI代码通常在rt.jar中,但是第三方的实现通常是放在classpath中.由于加载rt.jar中的类加载器是bootstrap类加载,无法加载到普通的classpath下面的类.所以这个时候,一个workaround的方法就是在调用前设置contextClassloader,然后在加载时,由contextClassloader去加载第三方实现.

在典型的JDBC驱动中,在以前的历史代码中,我们通常会看到这样一句话:Class.forName(JDBC_DRIVER) ,这个是因为JDBC规范定义了一堆接口,在其getConnection时,需要获得指定的第三方数据库驱动.但是它没有读什么固定的配置啥的,而是在接口中定义了DriverManager.registerDriver()方法,由第三方驱动调用后,将驱动信息放到内存中.而之所以要写这句,是因为Class.forName(JDBC_DRIVER)会触发该jdbc驱动实现类的初始化,而通常会在该驱动中的static块中去调用DriverManager.registerDriver()方法,从而最终实现驱动的正确加载.在JDBC4.0驱动实现中,通过jdk1.6的 ServiceLoader机制,可以自动加载第三方驱动,不需要再执行Class.forName(JDBC_DRIVER)了.

clazz.getResource 会区分"/name"和"name",因为其内部实现会调用reslove方法,最终再调用classloader.getResource方法.再补充一句,由于clazz.getResource的返回值是URL,而有些同学为了获得路径,会显示对URL进行toString后,然后在针对file等进行判断;其实,比较好的方式是调用new File(URL.toURI).getPath 方法.

# 参考
http://www.javaworld.com/javaworld/javaqa/2003-06/01-qa-0606-load.html


{% include JB/setup %}
