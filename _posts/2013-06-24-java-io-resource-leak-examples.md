---
layout: post
title: "JAVA IO 句柄泄露典型例子以及一些TIPS"
description: ""
category: 
tags: [J2SE-IO]
---

    1.open stream //没有close
	2.open stream 
	  close stream //没有在TCF中close
	3.try
	  { open stream}
	  catch exception
	  { deal exception}
	  finally
	  { do xxxxx; //可能会抛出异常 
	  	close stream;
	  }
	4.try{
	  in = openInputStream
	  out= openOutputStream;
	  }
	  catch exception
	  {deal exception}
	  finally
	  {
	  	try{
	  		in.close;  //in.close可能抛出异常,导致out无法close
	  		out.clsoe;
	  	}catch exception
	  	{deal exception}	  
	  }
	  5.
	    try{
	    InputStream in = null;
	   try
	    { in = openStreamA;
	      do XXXX; //可能抛出异常,但是已经打开流了,
	    }catch Exception{
	      in = openStreamB;//导致openStreamA中流泄露
	      do XXXX;
	    }}
	    catch(Exception)
	    {deal exception}
	    finally
	    {in.close}
	   6. try{
	    InputStream in = null;
	    for each{
	    in = openStream;
	    do xxxx;
	    }
	    catch(Exception)
	    {deal exception}
	    finally
	    {in.close}//打开N个流,却只关闭了最后一个流
	   7.打开socket连接,没有关闭socket
 
	
TIPS:

0. 可以通过findbugs静态检查工具或者代码走读方式来发现IO流未关闭(通过UE指定正则表达式,关键字:stream|reader|socket,并使用perl 引擎)
1. 不需要关闭底层流
2. JAVA GC时对流的特殊操作
3. io 泄露时句柄观察可使用lsop 命令和 或观察/proc/sys/fs/file-nr 
4. 一般流操作建议使用Apache IOUtils 
5. 除非是工具类,否则一般不建议方法参数或者返回值为Stream对象,要注明由谁来关闭流.
	

{% include JB/setup %}
