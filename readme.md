# Heeb-WebServer
在Linux平台下，用C++封装了std::thread,socket,epoll等内容，实现了支持格式化输出的同步日志，以及在封装std::thread的基础上实现了线程池。并初步实现了Reactor模型的Web服务器，目前正在逐步调试和优化。

## 功能
* 利用C++标准库的stream流式输入输出、实现了支持格式化同步记录日志；
* 对C++标准库中std::thread进行进一步封装为Thread，为适应服务器要求，增加了线程名和启动时间等信息；
* 在Thread的基础上C++11新特性实现了可以指定线程容量的线程池ThreadPool;
* 对Linux下的socket进行封装，将socket相关的bind listen send等函数封装到Socket内部，从而实现了面向对象的Socket类;
* 对Linux下的epoll进行封装，实现了面向对象的epoll创建，描述法添加和监听等功能；
* 利用std::vector\<char\>封装了自动增长的读写缓存区Buffer；
* 利用正则表达式和状态机实现了Http请求报文的解析和响应报文的生成；
* 利用Epoll(ET)和ThreadPool实现了Reactor高并发模型。

## 环境要求
* Linux
* C++11

## 目录树
```
├── build
├── CMakeLists.txt
├── debug
├── include 头文件
│   ├── Buffer.h
│   ├── Epoll.h
│   ├── HttpConnection.h
│   ├── Log.h
│   ├── Reactor.h
│   ├── Socket.h
│   ├── Thread.h
│   ├── ThreadPool.h
│   ├── Timer.h
│   └── WebServer.h
├── log     日志
├── resources 静态资源
│   ├── images
│   ├── music
│   ├── video
│   ├── css
│   ├── js
│   ├── fonts
│   ├── index.html
│   ├── login.html
│   ├── music.html
│   ├── picture.html
│   ├── register.html
│   ├── video.html
│   └── welcome.html
└── src     源文件
    ├── Buffer.cpp
    ├── Epoll.cpp
    ├── HttpConnection.cpp
    ├── LogAppender.cpp
    ├── Logger.cpp
    ├── LogLayout.cpp
    ├── LogManager.cpp
    ├── main.cpp
    ├── Reactor.cpp
    ├── Socket.cpp
    ├── TestLog.cpp
    ├── TestThreadPool.cpp
    ├── Thread.cpp
    ├── ThreadPool.cpp
    ├── Timer.cpp
    └── WebServer.cpp
```

## 项目生成和运行
```
~/Heeb$ cmake ..
~/Heeb/build$ make
~/Heeb$ ./build/WebServer
```

## Demo
![获取网页](https://i.loli.net/2021/08/13/nZwy6zL9MFYIBb1.gif)

## ToDo
* 增加日志的异步记录功能，采用无锁队列进行记录
* 调试完善服务器的各项基本功能，并进行压力测试
* 增加mysql数据库部分，实现注册、登陆等功能

## 致谢
Linux高性能服务器编程，游双著  
Unix网络编程 卷1 套接字联网API， W.Richard Stevens等著  
感谢下面大佬的代码参考和静态资源  
[@markparticle](https://github.com/markparticle/WebServer)  
[@qinguoyi](https://github.com/qinguoyi/TinyWebServer)  
[@lzpong](https://github.com/lzpong/threadpool)  
[@sylar-yin](https://github.com/sylar-yin/sylar)  


