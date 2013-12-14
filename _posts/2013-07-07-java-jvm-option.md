---
layout: post
title: "java jvm option"
description: ""
category: 
tags: []
---
maxDirectMemory

 
UseSerialGC is "Serial" + "Serial Old"
UseParNewGC is "ParNew" + "Serial Old"
UseConcMarkSweepGC is "ParNew" + "CMS" + "Serial Old". "CMS" is used most of the time to collect the tenured generation. "Serial Old" is used when a concurrent mode failure occurs.
UseParallelGC is "Parallel Scavenge" + "Serial Old"
UseParallelOldGC is "Parallel Scavenge" + "Parallel Old"

 R大的iteye 参数坑

====


http://www.oracle.com/technetwork/java/javase/tech/vmoptions-jsp-140102.html

http://kenwublog.com/docs/java6-jvm-options-chinese-edition.htm

{% include JB/setup %}
