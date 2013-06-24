---
layout: post
title: "Spring Framework 源码阅读及过程思考+吐槽"
description: "Spring Framework Source Study"
category: Spring Framework
tags: [Spring Framework]
---
### 背景
很多人都说JAVA企业级应用没啥技术含量，玩不了大数据的技术人员是很杯具的存在。

Spring Framework（下文简称为Spring）提供了很多魔法，使得很多开发变成Action->Service->DAO+一堆Interface（我讨厌很多莫名其妙的接口）,just make skins for dababase.

当然，Spring 的框架实现还是很强大的，虽然里面没啥高深算法，但是的确使开发工作简便了不少（Joel曾说，OOP太简单了，不足以分辨出优秀的程序员）。对一个对一切未知事物充满好奇心的人来说，尤其是一个没怎么见过市面的人来说，理解Spring的内部实现还是很有必要的。

本文主要分为4个章节，分别是IOC，AOP,Transaction，Remoting

注：本文不涉及Spring Framework 源码内部实现框架细节，细节部分请阅读本文最后的参考章节。
 
### IOC

对象之间的依赖关系由IOC Container来管理，本来应该是直接是代码中直接new另外一个class的，结果变成是在IOC容器中，通过配置文件来配置对象依赖关系。至于为什么需要IOC，主要是因为复杂对象之间的依赖关系通过IOC管理起来比较简单，不需要手写大量代码。另外，其他一些红利，比如说1.只要改配置文件，不需要改代码就完成对象依赖修改 2.提供方便的单例类 。

既然说到配置文件，那么就必须涉及到配置文件的**解析**，解析前需要**校验**文件合法性，解析后的内容再与相应的java类**映射**。但是在开始解析文件之前，首先要搞清楚2个问题：1.**解析的文件在哪里**，是指定的绝对路径还是相对路径还是classpath 2.**文件的格式是什么**，是xml还是properties，甚至是文本文件还是二进制文件。如果是xml，它相应的dtd或xsd是什么？搞清楚上述问题后，我们再进行选择解析组件。就XML而言，简单的文件直接通过JAXB/Digester来解决；复杂的一般是使用SAX来解决,例如Spring和Activiti（流程引擎）。

回到正题，我们看看Spring的解析相关的类。
Resource 的实现类由多个，典型的如下ClassPathResource，UrlResource，ByteArrayResource。

翻译区分已完结,未完结
有些疑问及时答复
把TED系列整理到一个合集中把
有个汇总简单说明系列,推荐学习的顺序
最后还是感谢下 :)

数学,计算机,物理,化学,
经济,政治,历史,哲学,心理


从这段代码看下void org.springframework.beans.factory.xml.XmlBeanFactoryTests.testInitMethodIsInvoked() throws Exception

		@Test
	public void testInitMethodIsInvoked() throws Exception {
		DefaultListableBeanFactory xbf = new DefaultListableBeanFactory();
		new XmlBeanDefinitionReader(xbf).loadBeanDefinitions(INITIALIZERS_CONTEXT);
		DoubleInitializer in = (DoubleInitializer) xbf.getBean("init-method1");
		// Initializer should have doubled value
		assertEquals(14, in.getNum());
	}

BeanDefinition --> AbstractBeanDefinition  ->GenericBeanDefinition  接口,抽象类,实现类,模板模式也经常看到

BeanDefinition 与XML的映射类，其重要方法： 

BeaFactory 与 Context的关系
1.2个步骤创建ioc ,一个步骤创建
2.直接从AC构造的代码来看,比复杂.

之所以不贴代码,主要是现在网页代码显示不好看,没有IDE看起来舒服,二是给出重点关注的地方,真正需要理解的话还是自己去看代码去.自己debug

需要核心实现思想，关键算法以及 关注扩展点 ，需要调试bug或进行代码修改时再进一步理解里面的繁杂细节。


做一些正确的事情比做其他小事情要容易的多.

	void org.springframework.context.support.AbstractApplicationContext.refresh() throws BeansException, IllegalStateException  --> ConfigurableListableBeanFactory org.springframework.context.support.AbstractApplicationContext.obtainFreshBeanFactory() -->void org.springframework.context.support.AbstractRefreshableApplicationContext.refreshBeanFactory() throws BeansException ,这个refreshBeanFactory()通过DefaultListableBeanFactory beanFactory = createBeanFactory();创建DefaultListableBeanFactory,可见context包括了beanfactory;然后接着调用了loadBeanDefinitions(beanFactory);加载资源配置文件,基本同 BeanFactory初始化过程;
	
	void org.springframework.context.support.ClassPathXmlApplicationContextTests.testSingleConfigLocation()







思考的问题影响你对问题的思考。在特定场景下，没有bug。在有限资源下，人们无法穷尽所有输入，所以还是可能存在bug的。

aop日志 典型的不切实际的演示，我们有个产品居然用了AOP进行打日志。。。

P26 beanFactory 与 context的区别,感觉看ApplicationContext类的注释就差不多了,

配置文件一般解析模式
定义 
简单的 jaxb
复杂的 sax，如 spring 和 activiti， 自定义标签 ，一个一个node来搞
支持多种配置文件获取，最后都转成 getResource。。，一般交给spring即可，参照activiti

sax解析，这个是体力活，但是xml解析命名实在比较操蛋，但是xml的后面的必须知道的知识，namespace，xsd，dtd 

自定义标签 
weak reference

解析完成后，形成一定的对象后，然后再刷新各个组件的关系，向spring的配置文件比较复杂。  

对象继承，实现层次较深，并不是很好。
3个接口顾名思义，能够比较准确的表单含义。
但是xx类，命名和该类的实际职责并不是严格匹配，看起来比较蛋疼。
所以，不需要记住这些类的主要职责，还是记住它主要做些什么，对我们的日常开发由什么作用，如何扩展。

核心类图（重要属性），重要方法，核心调用时序

jaxb简单，略；spring的解析比较简单，
整数加减乘除 编译原理实现 支持括号 后缀表达式
String.intern 的不同版本的表现,1.7中还有方法区吗?

BeanFactory
ApplicationContext
FactoryBean vs BeanFactory
DefaultListableBeanFactory
XmlBeanDefinitionReader

BeanDefinitionRegistry

void org.springframework.beans.factory.xml.DefaultBeanDefinitionDocumentReader.parseBeanDefinitions(Element root, BeanDefinitionParserDelegate delegate)
这里定义了解析扩展标签的类

### 参考

[Why need IOC](http://stackoverflow.com/questions/871405/why-do-i-need-an-ioc-container-as-opposed-to-straightforward-di-code)

[Spring 框架的设计理念与设计模式分析](http://www.ibm.com/developerworks/cn/java/j-lo-spring-principle/) 

[Spring 技术内幕](http://book.douban.com/subject/10470970/) 





ResourceLoader 资源的 定位 载入 注册 （spring 技术内幕）
bean 演员 context 舞台 core 工具类 （xulingbo）

jvm启动参数说法
回收算法细节

一般开源框架的特点 
1.支持多种配置方式，注解，Resource loader
2.提供事件（生命周期管理）和拦截器机制 
3.自定义runtime 异常 ，完善的debug日志（出问题时，能通过日志来定位，而不要第一时间来debug）
4.大型系统的 运维系统 （系统监控，互联网的快速发布，后台管理系统，回滚，服务降级）
代码管理
系统安全
5.性能测试 
6 用户行为分析，物流，仓库管理系统，热门商品预测，最短最佳成本路径
网易公开课
7 domain，自建，cdn -- nginx --  
8.服务无状态 ，减少锁， Btree ，lsm tree ，july blog，算法导论 请教 
9.如何计算TPMC（需要计算吗，实际中根本不靠谱。）？
集群，分布式，服务框架，远程通信框架，异步通信组件，序列化协议，
-XX:MaxDirectMemorySize 
restful api，
lvs 
zookeeper
nosql
ml
HashSet 是对HashMap的简单封装
activiti 代码解读
how tomcat works
用taobao开源框架搭建高可用 mini taobao应用，包括前端，后端。
Java 语言 ,虚拟机 规范  

研究spring的source预备知识：
设计模式，IOC,AOP的常见用法
XML解析（DTD ,XSD,**）
从单元测试入手，入口类 
void org.springframework.beans.factory.xml.XmlBeanFactoryTests.testInitMethodIsInvoked() throws Exception



昨晚梦里的一些东西：
有符号数，无符号数
C与汇编的关系
JAVA与C的关系
GC
BYTECODE
编译原理，

逻辑概念：内存和线程 相互作用，

原理是内功心法，最终解决方案才是能力体现，招式。经验

阅读开源代码的注意事项：
关注类的继承关系，spring的类继承还是有点复杂。
如果最终实现类的属性变化，主要关注里面的自定义类，不要过多考虑map，boolean的值 

写一篇“当你想玩电子游戏时”

大牛们在学习
同学们在锻炼身体
电脑里由很多资料需要整理（平时太乱了）
做些家务
做些体操
看几部以前未看的电影： lc的最新推荐的电影
看一些不太费脑的科普类文章
看一些保健书
看看远方风景，放松下眼睛。

 在计算机技术领域拥有扎实的技术功底，尤其在数据结构、算法和代码、软件设计方面功力深厚； 
具有丰富的使用C/C++或Java编程经验，两者皆熟练尤佳，在面向对象技术方面有较强经验者优先； 
在大规模系统软件的设计和开发方面有数年经验，对Unix/Linux有深入了解； 
如果有使用Python或Javascript/AJAX编程、数据库设计和SQL、TCP/IP以及网络编程等方面的经验，会进一步加深我们对您的兴趣；


读出channel的数据,然后写到buffer 中 (channel.read)



{% include JB/setup %}
