#pragma once
#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <vector>
#include <initializer_list>
#include "Thread.h"
#include <mutex>
#include <atomic>
#include <queue>
#include <functional>
#include <condition_variable>
#include <future>
#include <stdexcept>

namespace Heeb {
    class ThreadPool {
    public:
        //构造、析构
        ThreadPool(std::initializer_list<std::string> ThreadNameList);
        ThreadPool(size_t n, const std::string& ThreadName);
        ~ThreadPool();
        //添加任务
        template<typename Fun, typename... Args>
        auto addTask(Fun&& f, Args&&... args)->std::future<decltype(f(args...))>;
        size_t taskQueueSize();
    private:
        //成员变量
        std::vector<Thread::ptr> threadPool;        //线程池
        std::queue<std::function<void()>> taskQueue;//任务队列
        std::mutex taskLock;                        //任务锁
        std::condition_variable taskLockCV;         //任务队列同步条件变量
        std::atomic<bool> isRun;                    //是否运行：只有运行的线程池才能添加任务
        std::atomic<int> freeThreadNum;             //空闲线程数量
        std::function<void()> ThreadFunction;       //线程函数
    };

    template<typename Fun, typename... Args>
    auto ThreadPool::addTask(Fun&& f, Args&&... args)->std::future<decltype(f(args...))> {
        if(!isRun) throw std::runtime_error("addTask error, ThreadPool is not running!");
        using returnType = decltype(f(args...));
        auto task=std::make_shared<std::packaged_task<returnType()>>(std::bind(std::forward<Fun>(f), std::forward<Args>(args)...));
        std::future<returnType> FunFuture = task->get_future();
        {   
            std::lock_guard<std::mutex> taskLockGuard(taskLock);
            taskQueue.emplace([task](){(*task)();});
        }
        taskLockCV.notify_one();
        return FunFuture;
    }

}

#endif