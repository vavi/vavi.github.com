---
layout: post
title: "Java类文件结构"
description: ""
category: 
tags: [深入理解JVM]
---
# 前言
  Java虚拟机在不同的平台上(Windows,Mac和Linux等)上面实现了不同的虚拟机,然后通过载入并执行一份与平台无关的文件来实现跨平台. 这份平台无关的文件就是本文所要说的"Java字节码"(Bytecode).
  
  ByteCode的文件结构不用于XML,它没有任何分隔符,其文件结构和C语言的struct,TCP的报文比较相似,它们通常将文件分成几个固定的类别,几乎每个类别都是固定长度的大小.如果某个类别不是固定长度的话,通常会有"内容长度"和"内容"这两个属性来描述.
  
# 正文
其实,我觉得这篇文章 [实例分析Java Class的文件结构](http://coolshell.cn/articles/9229.html)说得很好了,我也不打算费太多笔墨来重述一遍.这里只是想说一声自己感兴趣的地方.

Java虚拟机之所以知道一个类中含有多少属性,方法等内容,是因为java源代码在通过编译器编译后,这些内容都被存放到Class文件中. 

类文件结构分为基本类型和复合类型(或者叫做表),基本类型的长度都是固定的,比如U2,U4(u2表示2 unsigned byte,2个字节).复合类型是有基本类型或者复合类型组成的. 

常量池实际个数大小= constant_pool_count-1;常量池索引0的值主要用来表示某些指向常量池的索引值数据在特定的情况下需要表达"不引用任何一个常量池项目"

常量池的每一项都是一个复合类型.这些复合类型的一个共同特征就是该复合类型的第一个字节(U1类型的)用于标识该复合类型的类型.

通过 javap -verbose ClassName 来分析Class文件字节码.

关于"JVM规范里CONSTANT_*的tag值为2的是啥？"这个问题,我特意请教了R大,详细链接见[JVM规范里CONSTANT_*的tag值为2的是啥](http://hllvm.group.iteye.com/group/topic/38367#249969)

用描述符描述方法时,按照参数列表,后返回值的顺序描述,参数列表按照参数的严格顺序放在一组小括号"()"之内

字段表集合不会列出从超类或者父接口中继承而来的字段,但是可能列出java代码中不存在的字段,比如在内部类中为了保持对外部类的访问性,会自动添加指向外部类实例的字段;与字段表集合相类似,方法表不会出现来自父类的方法信息(除非该方法被子类"override"),但是由有可能出现代码中不存在的方法,比如类构造器"<clinit>" 和实例构造器"<init>" 

在Java语言中,要overload一个方法,除了要和原方法具有相同的方法名称之外,还要求必须拥有一个与原方法不同的特征签名.特征签名是指一个方法中各个参数在常量池中的字段符号引用的集合.也就是说方法的返回值不会包含在特征签名之中.因此java语言中无法仅仅依靠返回值的不同来对一个已有方法进行重载(这让我想起以前微博上的一个段子,什么情况下一个类不能同时实现两个接口,其实这里说的也就是,2个接口的方法名称相同,参数相同,但是返回值不同).但是在Class文件中,特征签名的范围更大一些,只要描述符不完全一致就行.


属性表集合不再要求各个属性表之间由严格的顺序,并且只要不和已有属性名重复,任何人实现的编译器都可以想属性表中写入自己定义的属性信息.JVM在运行时会忽略掉它不认识的属性.

方法中的代码放在方法的属性表中的一个名为"Code"的属性,但是接口或者抽象类的方法中就不存在Code属性.Code属性是Class文件中最重要的一个属性,如果将一个Java程序中的信息分为代码(Code,方法体里面的代码)和元数据(Metadata,包含类,字段,方法定义及其他信息). Code属性中的一个"code"属性用于存储一个字节码指令的一系列字节流.每个字节码指令一个单字节长度.Code属性中的一个"code_length"属性虽然是U4类型的长度值,但是java虚拟机限制了一个方法不允许超过655535条字节码指令. 

实例方法存在隐藏参数this,静态方法不存在隐藏参数this

在jdk1.4.2之前的javac 编译器采用了jsr和ret指令实现了finally语句,但是在1.4.2之后就已经改为异常表实现了.

---

##参考

http://docs.oracle.com/javase/specs/jvms/se7/html/jvms-4.html

深入理解JAVA虚拟机,周志明著.



{% include JB/setup %}
