#pragma once 
#ifndef _REACTOR_H_
#define _REACTOR_H_

#include "Epoll.h"
#include <unordered_map>
#include <functional>
#include <mutex>
#include <atomic>
#include "ThreadPool.h"

namespace Heeb {
    //基于epoll事件的reactor
    class EpollReactor {
    public:
        typedef std::shared_ptr<EpollReactor> ptr;
        //创建reactor并设置监听事件
        EpollReactor();
        //监听事件：增删改 :epoll为线程安全的，无需额外加锁.并且只有构造函数中设置的reactor才会被监听到.
        bool AddFd(Socket::ptr msock, uint32_t event);
        bool ModFd(Socket::ptr msock, uint32_t event);
        bool DelFd(Socket::ptr msock);
        //事件处理：增删改
        bool AddHanle(uint32_t event, std::function<bool(EpollData*)> handle);
        bool ModHanle(uint32_t event, std::function<bool(EpollData*)> handle);
        bool DelHanle(uint32_t event);
        //reactor运行控制
        void Run(int sacnTime=100);
        void RunInThreadPool(ThreadPool* thread_pool, int sacnTime=100);
        void Stop();
        //任务多线程管理
        void EraseTaskMap(Socket::ptr sock);

    private:
        Epoll m_epoll;
        std::unordered_map<uint32_t, std::function<bool(EpollData*)>> m_handle;
        std::unordered_map<Socket::ptr, std::future<bool>> taskMap;
        std::atomic<bool> is_Run;
        std::mutex m_hanleMutex;
    };
}


#endif