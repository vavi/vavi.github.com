---
layout: post
title: "JDK动态代理原理剖析"
description: "JDK动态代理原理剖析"
category: J2SE 
tags: [J2SE]

---

昨天被问了个问题，问题的大意是这样的：为什么 `Proxy.newProxyInstance(ClassLoader loader, Class<?>[] interfaces, InvocationHandler h)`方法的3个参数是这样的定义的？笔者一阵语塞，好生郁闷。在这里补充一下，记录下对这个问题的解答。 

## 基本样例
### 接口类
	
	package com.vavi.proxy;

	public interface Sleepable {
    void sleep();

    void eat();
	}  
	
### 实现类
	package com.vavi.proxy;

	public class Person implements Sleepable {

    public void sleep() {
	System.out.println("He is sleeping");
    }

    public void eat() {
	System.out.println("He is eating");

    }

}

### InvocationHandler实现类

	package com.vavi.proxy.dynaimc.jdk;

	import java.lang.reflect.InvocationHandler;
	import java.lang.reflect.Method;

	public class PersonDynamicJDKProxyHandler implements InvocationHandler {
    private final Object targetObject;

    public PersonDynamicJDKProxyHandler(Object targetObject) {
	this.targetObject = targetObject;
    }

    @Override
    public Object invoke(Object proxy, Method method, Object[] args)
	    throws Throwable {
	if (method.getName().equals("eat")) {

	    System.out.println("wash hands before eating");
	    method.invoke(targetObject, args);
	    System.out.println("ready to sleep now..");
	} else {

	    System.out.println("take off clothes");
	    method.invoke(targetObject, args);
	    System.out.println("sweet dream now..");

	}

	return null;
    }
	}

### 客户端类
	package com.vavi.proxy.dynaimc.jdk;

	import java.io.FileOutputStream;
	import java.io.IOException;
	import java.lang.reflect.Proxy;

	import org.junit.Test;

	import com.vavi.proxy.Person;
	import com.vavi.proxy.Sleepable;

	public class TestPersonDynamicJDKProxy {
    @Test
    public void testProxy() throws Exception {

	// System.getProperties().put(
	// "sun.misc.ProxyGenerator.saveGeneratedFiles", true);
	Person person = new Person();
	PersonDynamicJDKProxyHandler handler = new PersonDynamicJDKProxyHandler(
		person);

	Sleepable proxy = (Sleepable) Proxy.newProxyInstance(person.getClass()
		.getClassLoader(), person.getClass().getInterfaces(), handler);

	// 获取代理类的字节码
	generateProxyClassFile();

	proxy.eat();
	proxy.sleep();

    }

    private void generateProxyClassFile() {
	byte[] classFile = sun.misc.ProxyGenerator.generateProxyClass(
		"$MyProxy", Person.class.getInterfaces());

	FileOutputStream out = null;
	String path = "/Users/ghj/startup/$MyProxy.class";
	try {
	    out = new FileOutputStream(path);
	    out.write(classFile);
	    out.flush();
	} catch (Exception e) {
	    e.printStackTrace();
	} finally {
	    try {
		out.close();
	    } catch (IOException e) {
		e.printStackTrace();
	    }
	}
    }
	}


## 源码分析

执行上述程序后，系统打印如下结果：
	
	wash hands before eating
	He is eating
	ready to sleep now..
	take off clothes
	He is sleeping
	sweet dream now..

现在程序运行正常，成功实现了动态代理的效果。但是这个为什么能够得到这样的效果呢？我们跟着一步步跟踪代码执行，首先发现最重要的是`(Sleepable) Proxy.newProxyInstance(person.getClass().getClassLoader(), person.getClass().getInterfaces(), handler);`这段代码。

 	public static Object Proxy.newProxyInstance(ClassLoader loader,
                                          Class<?>[] interfaces,
                                          InvocationHandler h)
        throws IllegalArgumentException
    {
        if (h == null) {
            throw new NullPointerException();
        }

        /*
         * Look up or generate the designated proxy class.
         */
        Class<?> cl = getProxyClass0(loader, interfaces); // 标记1
        /*
         * Invoke its constructor with the designated invocation handler.
         */
        try {
            final Constructor<?> cons = cl.getConstructor(constructorParams);// 标记2
            final InvocationHandler ih = h;
            SecurityManager sm = System.getSecurityManager();
            if (sm != null && ProxyAccessHelper.needsNewInstanceCheck(cl)) {
                // create proxy instance with doPrivilege as the proxy class may
                // implement non-public interfaces that requires a special permission
                return AccessController.doPrivileged(new PrivilegedAction<Object>() {
                    public Object run() {
                        return newInstance(cons, ih);//标记3
                    }
                });
            } else {
                return newInstance(cons, ih);//标记3
            }
        } catch (NoSuchMethodException e) {
            throw new InternalError(e.toString());
        }
    }

在上面代码中，标记1生成代理类的class对象，标记2获得带有这个参数`constructorParams`的构造器，标记3完成对象实例化。 需要提前说明的是，由于在Proxy类中，硬编码了`private final static Class[] constructorParams = { InvocationHandler.class };`这个属性值，所以这个一定程度了约束了我们必须要和`InvocationHandler`打交道了。并且隐含在代理类中，有一个带有`constructorParams`参数的构造器。这个在后文也会提及到。
        
下面接着看标记1内部实现（笔者删除了大量注释），中间进行了一些安全校验，接口个数校验，重复的接口名称等校验。

	private static Class<?> getProxyClass0(ClassLoader loader,
                                           Class<?>... interfaces) {
        SecurityManager sm = System.getSecurityManager();
        if (sm != null) {
            final int CALLER_FRAME = 3; // 0: Reflection, 1: getProxyClass0 2: Proxy 3: caller
            final Class<?> caller = Reflection.getCallerClass(CALLER_FRAME);
            final ClassLoader ccl = caller.getClassLoader();
            checkProxyLoader(ccl, loader);
            ReflectUtil.checkProxyPackageAccess(ccl, interfaces);
        }

        if (interfaces.length > 65535) {
            throw new IllegalArgumentException("interface limit exceeded");
        }

        Class<?> proxyClass = null;

        /* collect interface names to use as key for proxy class cache */
        String[] interfaceNames = new String[interfaces.length];

        // for detecting duplicates
        Set<Class<?>> interfaceSet = new HashSet<>();

        for (int i = 0; i < interfaces.length; i++) {
            
            String interfaceName = interfaces[i].getName();
            Class<?> interfaceClass = null;
            try {
                interfaceClass = Class.forName(interfaceName, false, loader);
            } catch (ClassNotFoundException e) {
            }
            if (interfaceClass != interfaces[i]) {
                throw new IllegalArgumentException(
                    interfaces[i] + " is not visible from class loader");
            }

            /*
             * Verify that the Class object actually represents an
             * interface.
             */
            if (!interfaceClass.isInterface()) {
                throw new IllegalArgumentException(
                    interfaceClass.getName() + " is not an interface");
            }

            /*
             * Verify that this interface is not a duplicate.
             */
            if (interfaceSet.contains(interfaceClass)) {
                throw new IllegalArgumentException(
                    "repeated interface: " + interfaceClass.getName());
            }
            interfaceSet.add(interfaceClass);

            interfaceNames[i] = interfaceName;
        }

         
        List<String> key = Arrays.asList(interfaceNames);

        /*
         * Find or create the proxy class cache for the class loader.
         */
        Map<List<String>, Object> cache;
        synchronized (loaderToCache) {
            cache = loaderToCache.get(loader);
            if (cache == null) {
                cache = new HashMap<>();
                loaderToCache.put(loader, cache);
            }
            /*
             * This mapping will remain valid for the duration of this
             * method, without further synchronization, because the mapping
             * will only be removed if the class loader becomes unreachable.
             */
        }

         
        synchronized (cache) {
            
            do {
                Object value = cache.get(key);
                if (value instanceof Reference) {
                    proxyClass = (Class<?>) ((Reference) value).get();
                }
                if (proxyClass != null) {
                    // proxy class already generated: return it
                    return proxyClass;
                } else if (value == pendingGenerationMarker) {
                    // proxy class being generated: wait for it
                    try {
                        cache.wait();
                    } catch (InterruptedException e) {
                         
                    }
                    continue;
                } else {
                     cache.put(key, pendingGenerationMarker);
                    break;
                }
            } while (true);
        }

        try {
            String proxyPkg = null;     // package to define proxy class in

            
            for (int i = 0; i < interfaces.length; i++) {
                int flags = interfaces[i].getModifiers();
                if (!Modifier.isPublic(flags)) {
                    String name = interfaces[i].getName();
                    int n = name.lastIndexOf('.');
                    String pkg = ((n == -1) ? "" : name.substring(0, n + 1));
                    if (proxyPkg == null) {
                        proxyPkg = pkg;
                    } else if (!pkg.equals(proxyPkg)) {
                        throw new IllegalArgumentException(
                            "non-public interfaces from different packages");
                    }
                }
            }

            if (proxyPkg == null) {
                // if no non-public proxy interfaces, use com.sun.proxy package
                proxyPkg = ReflectUtil.PROXY_PACKAGE + ".";
            }

            { 
                long num;
                synchronized (nextUniqueNumberLock) {
                    num = nextUniqueNumber++;
                }
                String proxyName = proxyPkg + proxyClassNamePrefix + num; // 标记1.1
                
                byte[] proxyClassFile = ProxyGenerator.generateProxyClass(
                    proxyName, interfaces); // 标记1.2
                try {
                    proxyClass = defineClass0(loader, proxyName, 
                        proxyClassFile, 0, proxyClassFile.length);// 标记1.3
                } catch (ClassFormatError e) {
                    
                    throw new IllegalArgumentException(e.toString());
                }
            }
            // add to set of all generated proxy classes, for isProxyClass
            proxyClasses.put(proxyClass, null);

        } finally {
            
            synchronized (cache) {
                if (proxyClass != null) {
                    cache.put(key, new WeakReference<Class<?>>(proxyClass));
                } else {
                    cache.remove(key);
                }
                cache.notifyAll();
            }
        }
        return proxyClass;
    }
         
标记1.1完成包名计算。 这里解释了为什么包名是类似`com.sun.proxy.$Proxy.NUM` 或者`PKG.$Proxy.NUM`的形式了。

标记1.2完成字节码数组拼接，这个稍后分析。

标记1.3完成字节码数组拼接，最终返回Class对象。

标记1.2的代码如下:

	public static byte[] ProxyGenerator.generateProxyClass(final String name,
                                            Class[] interfaces)
    {
        ProxyGenerator gen = new ProxyGenerator(name, interfaces);
        final byte[] classFile = gen.generateClassFile(); //标记1.2.1

        if (saveGeneratedFiles) { //标记1.2.2
            java.security.AccessController.doPrivileged(
            new java.security.PrivilegedAction() {
                public Object run() {
                    try {
                        FileOutputStream file =
                            new FileOutputStream(dotToSlash(name) + ".class");
                        file.write(classFile);
                        file.close();
                        return null;
                    } catch (IOException e) {
                        throw new InternalError(
                            "I/O exception saving generated file: " + e);
                    }
                }
            });
        }

        return classFile;
    } 

在上面代码的标记1.2.1中完成了实际的字节码拼接操作。 

在上面代码的标记1.2.2中，使用了这个saveGeneratedFiles变量。而这个变量是这么定义的` private final static boolean saveGeneratedFiles =  java.security.AccessController.doPrivileged( new GetBooleanAction("sun.misc.ProxyGenerator.saveGeneratedFiles")).booleanValue();`。这个参数可以帮助我们把字节码写到文件中了。

现在，接着看下标记1.2.1处的代码。从下面的代码我们可以看到。该方法先后完成了hashCodeMethod，equalsMethod，toStringMethod的数据准备，所有接口的所有方法。在标记1.2.1.1处完成了带InvocationHandler参数的构造器和静态代码块的数据准备。

为避免正文过长，我把`generateConstructor`和`generateStaticInitializer`挪到附录章节。


private byte[] generateClassFile() {

        /* ============================================================
         * Step 1: Assemble ProxyMethod objects for all methods to
         * generate proxy dispatching code for.
         */

        /*
         * Record that proxy methods are needed for the hashCode, equals,
         * and toString methods of java.lang.Object.  This is done before
         * the methods from the proxy interfaces so that the methods from
         * java.lang.Object take precedence over duplicate methods in the
         * proxy interfaces.
         */
        addProxyMethod(hashCodeMethod, Object.class);
        addProxyMethod(equalsMethod, Object.class);
        addProxyMethod(toStringMethod, Object.class);

        /*
         * Now record all of the methods from the proxy interfaces, giving
         * earlier interfaces precedence over later ones with duplicate
         * methods.
         */
        for (int i = 0; i < interfaces.length; i++) {
            Method[] methods = interfaces[i].getMethods();
            for (int j = 0; j < methods.length; j++) {
                addProxyMethod(methods[j], interfaces[i]);
            }
        }

        /*
         * For each set of proxy methods with the same signature,
         * verify that the methods' return types are compatible.
         */
        for (List<ProxyMethod> sigmethods : proxyMethods.values()) {
            checkReturnTypes(sigmethods);
        }

        /* ============================================================
         * Step 2: Assemble FieldInfo and MethodInfo structs for all of
         * fields and methods in the class we are generating.
         */
        try {
            methods.add(generateConstructor()); //标记1.2.1.1

            for (List<ProxyMethod> sigmethods : proxyMethods.values()) {
                for (ProxyMethod pm : sigmethods) {

                    // add static field for method's Method object
                    fields.add(new FieldInfo(pm.methodFieldName,
                        "Ljava/lang/reflect/Method;",
                         ACC_PRIVATE | ACC_STATIC));

                    // generate code for proxy method and add it
                    methods.add(pm.generateMethod());
                }
            }

            methods.add(generateStaticInitializer()); //标记1.2.1.2

        } catch (IOException e) {
            throw new InternalError("unexpected I/O Exception");
        }

        if (methods.size() > 65535) {
            throw new IllegalArgumentException("method limit exceeded");
        }
        if (fields.size() > 65535) {
            throw new IllegalArgumentException("field limit exceeded");
        }

        /* ============================================================
         * Step 3: Write the final class file.
         */

        /*
         * Make sure that constant pool indexes are reserved for the
         * following items before starting to write the final class file.
         */
        cp.getClass(dotToSlash(className));
        cp.getClass(superclassName);
        for (int i = 0; i < interfaces.length; i++) {
            cp.getClass(dotToSlash(interfaces[i].getName()));
        }

        /*
         * Disallow new constant pool additions beyond this point, since
         * we are about to write the final constant pool table.
         */
        cp.setReadOnly();

        ByteArrayOutputStream bout = new ByteArrayOutputStream();
        DataOutputStream dout = new DataOutputStream(bout);

        try {
            /*
             * Write all the items of the "ClassFile" structure.
             * See JVMS section 4.1.
             */
                                        // u4 magic;
            dout.writeInt(0xCAFEBABE);
                                        // u2 minor_version;
            dout.writeShort(CLASSFILE_MINOR_VERSION);
                                        // u2 major_version;
            dout.writeShort(CLASSFILE_MAJOR_VERSION);

            cp.write(dout);             // (write constant pool)

                                        // u2 access_flags;
            dout.writeShort(ACC_PUBLIC | ACC_FINAL | ACC_SUPER);
                                        // u2 this_class;
            dout.writeShort(cp.getClass(dotToSlash(className)));
                                        // u2 super_class;
            dout.writeShort(cp.getClass(superclassName));

                                        // u2 interfaces_count;
            dout.writeShort(interfaces.length);
                                        // u2 interfaces[interfaces_count];
            for (int i = 0; i < interfaces.length; i++) {
                dout.writeShort(cp.getClass(
                    dotToSlash(interfaces[i].getName())));
            }

                                        // u2 fields_count;
            dout.writeShort(fields.size());
                                        // field_info fields[fields_count];
            for (FieldInfo f : fields) {
                f.write(dout);
            }

                                        // u2 methods_count;
            dout.writeShort(methods.size());
                                        // method_info methods[methods_count];
            for (MethodInfo m : methods) {
                m.write(dout);
            }

                                         // u2 attributes_count;
            dout.writeShort(0); // (no ClassFile attributes for proxy classes)

        } catch (IOException e) {
            throw new InternalError("unexpected I/O Exception");
        }

        return bout.toByteArray();
    }    
    


我们最后使用反编译工具JAD-UI看下生成的代理对象字节码。完整的代码见附录，这里关注下里面的eat()方法。该方法内部的this.h属性就是InvocationHandler的实现类。然后调到invoke方法，完成了最终的执行。

 	public final void eat()

    {
	try {
	    this.h.invoke(this, m3, null); 
	    return;
	} catch (Error localError) {
	    throw localError;
	} catch (Throwable localThrowable) {
	    throw new UndeclaredThrowableException(localThrowable);
	}
    }

## 总结
1. 最后，我们再回头看看本篇提的问题。这3个参数分别解决了如下几个问题：
	1. `ClassLoader loader` 解决了使用什么classloader来加载这个代理类
    2. `Class<?>[] interfaces` 首先约束了JDK动态代理机制是基于接口实现的，它要求我们被代理的类必须实现相应的接口；其次JDK动态代理机制会帮我们完成接口方法的代理方法的实现，并通过硬编码把代理职责委托给了`InvocationHandler h` 这个参数。
    3. `InvocationHandler h` 完成了实际的代理职责。`InvocationHandler.invoke(Object proxy, Method method, Object[] args)`的3个参数：
    	1. proxy是生成的代理实例，里面不包含target对象实例。所以，我们一般在实现InvocationHandler接口时，会通过构造方法传入target对象。
    	2. method和args 分别对应了 target对象的方法和参数
    	3. 在`InvocationHandler.invoke`内部再利用反射完成target对象的方法执行。
2. 在源码面前，一切毫无遁形。
3. 知其然，尽量要知其所以然。
 
## 附录：
1. [ProxyGenerator源码](http://grepcode.com/file/repository.grepcode.com/java/root/jdk/openjdk/6-b14/sun/misc/ProxyGenerator.java#ProxyGenerator.generateConstructor%28%29)

2.构造器数据准备
     
     private MethodInfo generateConstructor() throws IOException {
        MethodInfo minfo = new MethodInfo(
            "<init>", "(Ljava/lang/reflect/InvocationHandler;)V",
            ACC_PUBLIC);

        DataOutputStream out = new DataOutputStream(minfo.code);

        code_aload(0, out);

        code_aload(1, out);

        out.writeByte(opc_invokespecial);
        out.writeShort(cp.getMethodRef(
            superclassName,
            "<init>", "(Ljava/lang/reflect/InvocationHandler;)V"));

        out.writeByte(opc_return);

        minfo.maxStack = 10;
        minfo.maxLocals = 2;
        minfo.declaredExceptions = new short[0];

        return minfo;
    }

3.静态块数据准备

    private MethodInfo generateStaticInitializer() throws IOException {
        MethodInfo minfo = new MethodInfo(
            "<clinit>", "()V", ACC_STATIC);

        int localSlot0 = 1;
        short pc, tryBegin = 0, tryEnd;

        DataOutputStream out = new DataOutputStream(minfo.code);

        for (List<ProxyMethod> sigmethods : proxyMethods.values()) {
            for (ProxyMethod pm : sigmethods) {
                pm.codeFieldInitialization(out);
            }
        }

        out.writeByte(opc_return);

        tryEnd = pc = (short) minfo.code.size();

        minfo.exceptionTable.add(new ExceptionTableEntry(
            tryBegin, tryEnd, pc,
            cp.getClass("java/lang/NoSuchMethodException")));

        code_astore(localSlot0, out);

        out.writeByte(opc_new);
        out.writeShort(cp.getClass("java/lang/NoSuchMethodError"));

        out.writeByte(opc_dup);

        code_aload(localSlot0, out);

        out.writeByte(opc_invokevirtual);
        out.writeShort(cp.getMethodRef(
            "java/lang/Throwable", "getMessage", "()Ljava/lang/String;"));

        out.writeByte(opc_invokespecial);
        out.writeShort(cp.getMethodRef(
            "java/lang/NoSuchMethodError", "<init>", "(Ljava/lang/String;)V"));

        out.writeByte(opc_athrow);

        pc = (short) minfo.code.size();

        minfo.exceptionTable.add(new ExceptionTableEntry(
            tryBegin, tryEnd, pc,
            cp.getClass("java/lang/ClassNotFoundException")));

        code_astore(localSlot0, out);

        out.writeByte(opc_new);
        out.writeShort(cp.getClass("java/lang/NoClassDefFoundError"));

        out.writeByte(opc_dup);

        code_aload(localSlot0, out);

        out.writeByte(opc_invokevirtual);
        out.writeShort(cp.getMethodRef(
            "java/lang/Throwable", "getMessage", "()Ljava/lang/String;"));

        out.writeByte(opc_invokespecial);
        out.writeShort(cp.getMethodRef(
            "java/lang/NoClassDefFoundError",
            "<init>", "(Ljava/lang/String;)V"));

        out.writeByte(opc_athrow);

        if (minfo.code.size() > 65535) {
            throw new IllegalArgumentException("code size limit exceeded");
        }

        minfo.maxStack = 10;
        minfo.maxLocals = (short) (localSlot0 + 1);
        minfo.declaredExceptions = new short[0];

        return minfo;
    }    


4.代理类反编译后的源码
	
	package com.vavi.proxy.dynaimc.jdk;

	import java.lang.reflect.Method;
	import java.lang.reflect.Proxy;
	import java.lang.reflect.UndeclaredThrowableException;

	import com.vavi.proxy.Sleepable;

	public final class MyProxy extends Proxy implements Sleepable {
    private static Method m1;
    private static Method m3;
    private static Method m0;
    private static Method m4;
    private static Method m2;

    public MyProxy()

    {
	super(paramInvocationHandler);
    }

    public final boolean equals()

    {
	try {
	    return ((Boolean) this.h.invoke(this, m1,
		    new Object[] { paramObject })).booleanValue();
	} catch (Error localError) {
	    throw localError;
	} catch (Throwable localThrowable) {
	    throw new UndeclaredThrowableException(localThrowable);
	}
    }

    @Override
    public final void eat()

    {
	try {
	    this.h.invoke(this, m3, null);
	    return;
	} catch (Error localError) {
	    throw localError;
	} catch (Throwable localThrowable) {
	    throw new UndeclaredThrowableException(localThrowable);
	}
    }

    @Override
    public final int hashCode()

    {
	try {
	    return ((Integer) this.h.invoke(this, m0, null)).intValue();
	} catch (Error localError) {
	    throw localError;
	} catch (Throwable localThrowable) {
	    throw new UndeclaredThrowableException(localThrowable);
	}
    }

    @Override
    public final void sleep()

    {
	try {
	    this.h.invoke(this, m4, null);
	    return;
	} catch (Error localError) {
	    throw localError;
	} catch (Throwable localThrowable) {
	    throw new UndeclaredThrowableException(localThrowable);
	}
    }

    @Override
    public final String toString()

    {
	try {
	    return ((String) this.h.invoke(this, m2, null));
	} catch (Error localError) {
	    throw localError;
	} catch (Throwable localThrowable) {
	    throw new UndeclaredThrowableException(localThrowable);
	}
    }

    static {
	try {
	    m1 = Class.forName("java.lang.Object").getMethod("equals",
		    new Class[] { Class.forName("java.lang.Object") });
	    m3 = Class.forName("com.vavi.proxy.Sleepable").getMethod("eat",
		    new Class[0]);
	    m0 = Class.forName("java.lang.Object").getMethod("hashCode",
		    new Class[0]);
	    m4 = Class.forName("com.vavi.proxy.Sleepable").getMethod("sleep",
		    new Class[0]);
	    m2 = Class.forName("java.lang.Object").getMethod("toString",
		    new Class[0]);
	    return;
	} catch (NoSuchMethodException localNoSuchMethodException) {
	    throw new NoSuchMethodError(localNoSuchMethodException.getMessage());
	} catch (ClassNotFoundException localClassNotFoundException) {
	    throw new NoClassDefFoundError(
		    localClassNotFoundException.getMessage());
	}
    }
	}    


% include JB/setup %}


 