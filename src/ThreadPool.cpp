#include "../include/ThreadPool.h"

namespace Heeb {

    ThreadPool::ThreadPool(std::initializer_list<std::string> ThreadNameList): isRun(true), freeThreadNum(0) {
        if(ThreadNameList.size() < 1) {
            isRun = false;
            throw(std::runtime_error("the ThreadPool's thread num should > 0"));
        }
        ThreadFunction=[this]() {
            while(isRun) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> taskUniLock(taskLock);
                    taskLockCV.wait(taskUniLock, [this](){return !isRun || !this->taskQueue.empty();});
                    if(!isRun && this->taskQueue.empty()) return; //线程池不运行（无法添加任务）且任务队列为空（任务全部执行完）
                    task = std::move(this->taskQueue.front());
                    taskQueue.pop();
                }    
                --freeThreadNum;
                task();
                ++freeThreadNum;     
            }
        };
        for(auto iter = ThreadNameList.begin(); iter != ThreadNameList.end(); ++iter) {
            Thread::ptr th = std::make_shared<Thread>(*iter);
            th->run(ThreadFunction);
            threadPool.emplace_back(th);
            ++freeThreadNum;
        }
    }
    ThreadPool::ThreadPool(size_t n, const std::string& ThreadName): isRun(true), freeThreadNum(0) {
        if(n < 1) {
            isRun = false;
            throw(std::runtime_error("the ThreadPool's thread num should > 0"));
        }
        ThreadFunction=[this]() {
            while(isRun) {
                {
                    std::unique_lock<std::mutex> taskUniLock(taskLock);
                    taskLockCV.wait(taskUniLock, [this](){return !isRun || !this->taskQueue.empty();});
                    if(!isRun && this->taskQueue.empty()) return; //线程池不运行（无法添加任务）且任务队列为空（任务全部执行完）
                    std::function<void()> task = std::move(this->taskQueue.front());
                    taskQueue.pop();
                    --freeThreadNum;
                    task();
                    ++freeThreadNum;
                }
            }
        };
        while(n>0) {
            Thread::ptr th = std::make_shared<Thread>(ThreadName);
            th->run(ThreadFunction);
            threadPool.emplace_back(th);
            ++freeThreadNum;
            --n;
        }
    }
    ThreadPool::~ThreadPool() {
        //线程池运行状态设为false，唤醒所有线程直至任务运行结束
        isRun=false;
        taskLockCV.notify_all();
        for(auto th:threadPool) th->join();
    }

    size_t ThreadPool::taskQueueSize() {
        size_t size;
        {
            std::lock_guard<std::mutex> lockGuard(taskLock);
            size=taskQueue.size();
        }
        return size;
    }
}