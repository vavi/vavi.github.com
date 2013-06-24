---
layout: post
title: "JAVA IO 句柄泄露典型例子以及一些注意事项"
description: ""
category: 
tags: [J2SE-IO]
---

    1.open stream
	2.open stream 
	  close stream
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
	  		in.close;
	  		out.clsoe;
	  	}catch exception
	  	{deal exception}
	  	
	  	
	  }
	  5.
	    try{
	    InputStream in = null;
	   if(ACondtion )
	    { in = openStreamA;
	      do XXXX; 
	    }else{
	      in = openStreamB;
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
	    {in.close}
	   7. 
	
	socket 
	
	一些常见问题
	1.是否需要关闭底层流
	2.JAVA gc时对流的特殊操作
	3.io 泄露时句柄观察
	5.如何进行代码走读(stream|reader|socket)
	6.使用ioutils 
	7.方法参数或者返回值为Stream对象
	

{% include JB/setup %}
