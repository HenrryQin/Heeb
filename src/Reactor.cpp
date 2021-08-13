#include "../include/Reactor.h"

namespace Heeb {

    EpollReactor::EpollReactor():m_epoll(), is_Run(false) {  }
    //监听事件：增删改 :epoll为线程安全的，无需额外加锁
    bool EpollReactor::AddFd(Socket::ptr msock, uint32_t event) {
        return m_epoll.AddFd(msock, event);
    }
    bool EpollReactor::ModFd(Socket::ptr msock, uint32_t event) {
        return m_epoll.ModFd(msock, event);
    }
    bool EpollReactor::DelFd(Socket::ptr msock) {
        return m_epoll.DelFd(msock);
    }
    //事件处理：增删改
    bool EpollReactor::AddHanle(uint32_t event, std::function<bool(EpollData*)> handle) {
        std::lock_guard<std::mutex> handleLock(m_hanleMutex);
        if(m_handle.find(event)!=m_handle.end()) {
            std::cout<<"EpollReactor: AddHandle() Error: event existed"<<std::endl;
            return false;
        }
        m_handle[event]=handle;
        return true;
    }
    bool EpollReactor::ModHanle(uint32_t event, std::function<bool(EpollData*)> handle) {
        std::lock_guard<std::mutex> handleLock(m_hanleMutex);
        if(m_handle.find(event)==m_handle.end()) {
            std::cout<<"EpollReactor: ModHandle() Warn: event doesn't exist"<<std::endl;
            return false;
        }
        m_handle[event]=handle;
        return true;
    }
    bool EpollReactor::DelHanle(uint32_t event) {
        std::lock_guard<std::mutex> handleLock(m_hanleMutex);
        if(m_handle.find(event)==m_handle.end()) {
            std::cout<<"EpollReactor: DelHandle() Warn: event doesn't exist"<<std::endl;
            return false;
        }
        m_handle.erase(event);
        return true;
    }
    //reactor运行控制
    void EpollReactor::Run(int sacnTime) {
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "Reactor.cpp", 46, "enter Run()", LogLevel::INFO);
        is_Run=true;
        //运行时加锁，不允许添加handle
        //std::lock_guard<std::mutex> handleLock(m_hanleMutex);
        while(is_Run) {
            epoll_event events[128];
            int size=m_epoll.Wait(events, 128, sacnTime);
            for(int i=0; i<size; ++i) {
                EpollData* mdata = (EpollData*)events[i].data.ptr;
                auto iter=m_handle.find(events[i].events);
                if(iter==m_handle.end()) {
                    LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "Reactor.cpp", 57, "UNKNOW EVENT"+std::to_string(events[i].events), LogLevel::INFO);
                    continue;
                }
                LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "Reactor.cpp", 60, "mainReactor start handle "+std::to_string(iter->first), LogLevel::INFO);
                std::cout<<iter->second(mdata)<<std::endl;
                LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "Reactor.cpp", 62, "mainReactor end handle "+std::to_string(iter->first), LogLevel::INFO);
            }
        }
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "Reactor.cpp", 59, "exit Run()", LogLevel::INFO);
    }

    void EpollReactor::RunInThreadPool(ThreadPool* thread_pool, int sacnTime) {
        is_Run=true;
        //运行时加锁，不允许添加handle
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "Reactor.cpp", 65, "enter RunInThreadPool()", LogLevel::INFO);
        //std::lock_guard<std::mutex> handleLock(m_hanleMutex);
        while(is_Run) {
            epoll_event events[128];
            int size=m_epoll.Wait(events, 128, sacnTime);
            for(int i=0; i<size; ++i) {
                EpollData* mdata = (EpollData*)events[i].data.ptr;
                bool flag=true;
                LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "Reactor.cpp", 77, "subReactor: event: "+std::to_string(events[i].events)+" cap", LogLevel::INFO);
                for(auto& p:m_handle) {
                    if((events[i].events) & (p.first)) {
                        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "Reactor.cpp", 80, "subReactor: event: "+std::to_string(p.first)+" handle", LogLevel::INFO);
                        //等待该socket下任务完成
                        auto iter=taskMap.find(mdata->sock);
                        if(iter!=taskMap.end()) {
                            bool result = iter->second.get();
                            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "Reactor.cpp", 86, "task return: "+std::to_string(result), LogLevel::INFO);
                        }
                        //添加任务                                        
                        taskMap[mdata->sock]=thread_pool->addTask(p.second, mdata);
                        flag=false;
                        break;
                    }
                }
                if(flag) {
                    LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "Reactor.cpp", 85, "UNKNOW EVENT"+std::to_string(events[i].events), LogLevel::INFO);
                }               
            }
        }
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "Reactor.h", 108, "exit RunInThreadPool()", LogLevel::INFO);
    }

    void EpollReactor::Stop() {
        is_Run=false;//与Run不在同一个线程中才能停止该reactor运行
    }

    //任务多线程管理
    void EpollReactor::EraseTaskMap(Socket::ptr sock) {
        taskMap.erase(sock);
    }
}