#pragma once
#ifndef _THREAD_H_
#define _THREAD_H_

#include <iostream>
#include <string>
#include <thread>
#include <ctime>
#include <functional>

namespace Heeb { 
    class Thread {
    public:
        typedef std::shared_ptr<Thread> ptr;
        //拷贝构造定义为删除的：即使不显式声明，拷贝构造也会是删除的：成员m_thread的拷贝构造函数是删除的
        Thread(const Thread&)=delete;
        //默认构造
        Thread(const std::string& threadName="NoNameThread"): m_name(threadName) { }
        //运行构造
        template<typename Fun, typename... Args>
        Thread(const std::string& threadName, Fun&& fun, Args&&... args): m_name(threadName), initClock(clock()) {
            setThreadFun(std::forward<Fun>(fun), std::forward<Args>(args)...);
            m_thread=std::thread(init, this);
        }
        //析构函数
        ~Thread() {
            if(m_thread.joinable()) m_thread.join();
        }
        //运行函数：会改变id并重新计时
        template<typename Fun, typename... Args>
        void run(Fun&& f, Args&&... args) {
            if(m_thread.joinable()) m_thread.join();
            initClock=clock();
            setThreadFun(std::forward<Fun>(f), std::forward<Args>(args)...);
            m_thread=std::thread(init, this);
        }
        //判断线程是否有效
        bool joinable(); 
        //等待线程运行结束
        void join();
        //获取线程参数
        const std::string& get_threadName();
        const std::thread::id get_id();
        clock_t get_elapse();
    private:
        std::string m_name;
        clock_t initClock;
        std::thread m_thread; 
        std::function<void()> m_fun;

        template<typename Fun, typename... Args> 
        void setThreadFun(Fun&& f, Args&&... args) {
            this->m_fun=std::bind(std::forward<Fun>(f), std::forward<Args>(args)...);
        }

        static void init(Thread* _thisThread);

    };

    class ThisThread {
    public:
        static thread_local Thread* thisThread; //当前线程
        static const std::string& GetName();    //获取当前线程名称
        static const std::thread::id GetId();   //获取当前线程ID
        static clock_t GetElapse();             //获取当前线程运行时间
    };
}



#endif