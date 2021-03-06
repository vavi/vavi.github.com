---
layout: post
title: "深入理解计算机系统 读书笔记 1.计算机系统漫游"
description: "csapp-reading-note 深入理解计算机系统 读书笔记 1.计算机系统漫游"
category: CSAPP
tags: [CSAPP]
---
# 什么是计算机系统
计算机系统是由硬件和系统软件组成的,它们共同工作来运行应用程序

### 信息是位+上下文

1字节(byte)=8位(bit).计算机系统种所有的信息--包括磁盘文件,内存中的程序和用户数据以及从网络上传输的数据,都是由若干个字节组成的.一个同样的字节序列,在不同的上下文中,可能表达出不同的意思;它可能是一个整数,浮点数,字符串或者机器指令

	在我们日常生活中,123这个简单的字符可以是数序中的阿拉伯数字,也可以是音乐中的曲谱.它们的含义截然不同.

### C语言编译过程

![alt text]({{ BASE_PATH }}/images/csapp1/compilation-system.png "Title")
 
预处理阶段(Pre-processor):cpp根据以字符#开头的指令,修改原始的C程序.比如hello.c种的第一行的`<# include <stdio.h>`指令就是告诉预处理器读取系统头文件studio.h的内容，并且将它直接插入到程序文本中。结果就得到另外一个C程序，通常是以.i作为文件扩展名

编译阶段（Complier）：CC1讲文本文件hello.i翻译成hello.s,它包含一个汇编语言程序。高级语言在编译时，通常转成汇编语言

汇编阶段（Assembler）:汇编器讲hello.s翻译成由机器语言指令组成的二进制文件hello.o。

链接阶段（Linker）：连接器将hello程序中用到的printf.o与hello.o合并，得到一个hello文件

	了解编译系统的工作原理，可以帮助我们优化程序性能，理解链接时发生的错误以及避免安全漏洞
	
### 系统的硬件组成

![alt text]({{ BASE_PATH }}/images/csapp1/hardware-organization.png "Title")

**总线**：总线是贯穿这个系统的一组电子管道，包括System Bus，Memeory Bus，I/O Bus，它携带字节并负责在各个部件间传递。

**I/O设备**：I/O设备是系统和外部世界联系的通道，主要包括鼠标，键盘，显示器，磁盘

**内存**：内存是一个临时存储设备，在处理器执行程序时，用来存放程序和程序处理的数据。从物理上来讲，内存是由一组动态随机存取存储器（DRAM）芯片组成的。从逻辑上来讲，内存是一个线性字节数组，每个字节都有自己的唯一的地址（即数组索引）

**处理器**：处理器时解释（或执行）内存中指令的引擎。处理器的核心是一个字长的存储设备（寄存器），称之为PC（Program Counter）。在任何时刻，PC都指向内存中的某条机器语言指令（即含有该指令的内存地址）。

从系统通电开始，到系统断电为止，处理器一直不断地执行PC指向的指令（从内存中读取指令，解释指令中的位，执行该指令指示的**简单操作**），然后再更新PC，使其指向下一条指令。

这些简单操作并不多，主要围绕着内存，寄存器文件和算术/逻辑单元（ALU）进行的。寄存器文件时一个小的存储设备，由一些1字长的的寄存器组成的，每个寄存器都有唯一的名字。ALU计算新的数据和地址值。CPU在指令的要求下，可能会执行如下操作：

* 加载：把一个字节或者一个字从内存复制到寄存器，以覆盖寄存器原来的内容
* 存储：把一个字节或者一个字从寄存器复制到内存的某个位置，以覆盖内存这个位置上原来的内容
* 操作：把两个寄存器的内容复制到ALU，ALU对这个字进行算术操作，并将结果存放到一个寄存器中，以覆盖该寄存器原来的内容。
* 跳转：从指令本身中抽取一个字，并将这个字复制到PC中，以覆盖PC中原来的值

### 高速缓存

![alt text]({{ BASE_PATH }}/images/csapp1/cache-memories.png "Title")

从hello程序的执行过程来看，该程序的机器指令最初时存放到磁盘上的，当程序加载时，它们被复制到内存中；当处理器运行程序时，指令又从内存复制到处理器。

从程序员角度来看，这些复制就是开销，降低了程序的运行速度。因此，系统设计者的一个主要目标就是使这些复制操作尽快地完成。

根据机械原理，较大的存储设备要比较小的存储设备运行得慢；而快速运行的设备造价远高于同类的低速设备。一个典型的寄存器文件只存储几百字节，而内存里可存放几十亿字节。然后，处理器从寄存器文件中读取数据的速度比从内存中读取快几乎100倍。针对这种处理器和内存之间的差异，系统设计者采用了更小，更快的存储设备，即**高速缓存器**，作为暂时的集结区域，用来存放处理器近期可能需要的信息。

### 存储设备

![alt text]({{ BASE_PATH }}/images/csapp1/memory-hierarchy-en.png "Title")

![alt text]({{ BASE_PATH }}/images/csapp1/memory-hierarchy-cn.png "Title")

如上图，不解释。

### 操作系统管理硬件

![alt text]({{ BASE_PATH }}/images/csapp1/layed-view-of-computer-system.png "Title")

我们把操作系统看成是应用程序和硬件之间插入的一层软件。所有应用程序对硬件的操作尝试都必须通过操作系统。

操作系统由两个基本功能：1.防止硬件被失控的应用程序滥用 2.向应用程序提供简单一致的机制来控制复杂而又通常大相径庭的低级硬件设备。操作系统通过基本的抽象概念（进程、虚拟存储器和文件）来实现着两个功能。

![alt text]({{ BASE_PATH }}/images/csapp1/abstraction-of-os.png "Title")

文件是对I/O设备的抽象表示

虚拟存储器是对内存和磁盘I/O设备的抽象表示 

进程是对处理器、内存和I/O设备的抽象表示

#### 进程

进程是操作系统对一个正在运行的程序的一种抽象。无论是在单核还是多核系统中，一个CPU看上去都是在并发地执行多个进程，这是通过处理器在进程间切换来实现的。操作系统实现这种交错执行的机制称为**上下文切换**

操作系统保持跟踪进程运行所需要的所有状态信息。这种状态，也就是上下文，它包含很多信息，例如PC和寄存器文件的当前值，以及内存的内容。在任何一个时刻，单处理器系统都只能执行一个进程的代码。当操作系统决定要把控制权从当前进程转移到某个新进程时，就会进行上下文切换，即保存当前进程的上下文，恢复新进程的上下文，然后将控制权传递给新进程。新进程就会上次停止的地方开始。

#### 线程

在现代操作系统中，一个进程实际上可以由多个称之为线程的执行单元组成，每个线程都运行在进程的上下文中，并共享同样的代码和全局数据。

#### 虚拟存储器

虚拟存储器是一个抽象概念，它为每个进程提供了一个假象，即每个进程都在独占地使用内存。每个进程看到的是一致的存储器，称之为**虚拟地址空间**。

*虚拟存储器的运作需要硬件和操作系统软件精密复杂的交互，在后面章节再详述。*

#### 文件

文件就是字节序列，仅此而已。每个I/O设备，包括磁盘，键盘，显示器，甚至网络，都可以视为文件。文件向应用程序提供了一个统一的视角，来看待系统中所有可能的各式各样的I/O设备

### 网络通信

系统漫游至此，我们一直把系统视为一个孤立的硬件和软件的结合体。实际上，现代操作系统经常通过网络和其他系统连接到一起。从一个单独的系统来看，网络可视为一个I/O设备。

系统在把一串字节从内存复制到网络适配器时，数据流经过网络到达另一台机器。相思地，系统可以读取从其他机器发来的数据，并把数据复制到自己的内存。




### 阅读本章TIPS
- 需要一些简单C语言知识,可以参考K&R这本书   [C程序设计语言](http://book.douban.com/review/1005566/ "C程序设计语言")
- 编译阶段4个步骤的GCC命令  [Linux GCC常用命令](http://www.cnblogs.com/ggjucheng/archive/2011/12/14/2287738.html "Linux GCC常用命令")




