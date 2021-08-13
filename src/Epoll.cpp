#include "../include/Epoll.h"

namespace Heeb {
    Epoll::Epoll(): is_valid(true) {
        m_epoll_fd = epoll_create1(0);
        if(m_epoll_fd<0) {
            std::cout<<"Error: epoll_fd<0"<<std::endl;
            is_valid=false;
        }
    }
    bool Epoll::AddFd(Socket::ptr msock, uint32_t event, EpollData::ptr eventData) {
        if(!is_valid) {
            std::cout<<"Epoll::AddFd() invalid epoll fd"<<std::endl;
            return false;
        }
        epoll_event e;
        e.events = event;
        e.data.ptr=eventData.get();
        int result=epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, msock->getsock(), &e);
        if(result==0) return true;
        else if(result==EEXIST) std::cout<<"Epoll::AddFd() error: fd is existed"<<std::endl;
        else if(result==EBADF) std::cout<<"Epoll::AddFd() error: invalid fd"<<std::endl;
        else std::cout<<"Epoll::AddFd() error: epoll_ctl return "<<result<<std::endl;
        return false;
    }

    bool Epoll::AddFd(Socket::ptr msock, uint32_t event) {
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "Epoll.cpp", 28, "enter AddFd()", LogLevel::INFO);
        std::lock_guard<std::mutex> mapLock(m_mapMutex);
        std::shared_ptr<EpollData> epolldata=std::make_shared<EpollData>(msock, event);
        //EpollData* epolldata=new EpollData(msock, event);   
        if(AddFd(msock, event, epolldata)) {
            int mfd=msock->getsock();
            m_events[mfd]=epolldata;
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "Epoll.cpp", 35, "AddFd() return true", LogLevel::INFO);
            return true;
        }
        LogManager::GetInstance()->log("WebServerLog", LogLevel::WARN, "Epoll.cpp", 38, "AddFd() return false", LogLevel::WARN);   
        return false;
    }

    bool Epoll::AddFd(int mfd, uint32_t event) {
        if(!is_valid) {
            std::cout<<"Epoll::AddFd() invalid epoll fd"<<std::endl;
            return false;
        }
        epoll_event e;
        e.events = event;
        e.data.fd=mfd;
        int result=epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, mfd, &e);
        if(result==0) return true;
        else if(result==EEXIST) std::cout<<"Epoll::AddFd() error: fd is existed"<<std::endl;
        else if(result==EBADF) std::cout<<"Epoll::AddFd() error: invalid fd"<<std::endl;
        else std::cout<<"Epoll::AddFd() error: epoll_ctl return "<<result<<std::endl;
        return false;
    }

    bool Epoll::ModFd(Socket::ptr msock, uint32_t event, EpollData::ptr eventData) {
        epoll_event e;
        e.events=event;
        e.data.ptr=eventData.get();
        int result=epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, msock->getsock(), &e);
        if(result == 0) return true;
        else std::cout<<"Epoll::ModFd() error: epoll_ctl return "<<result<<std::endl;
        return false;
    }

    bool Epoll::ModFd(Socket::ptr msock, uint32_t event) {
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "Epoll.cpp", 69, "enter ModFd()", LogLevel::INFO);
        std::lock_guard<std::mutex> mapLock(m_mapMutex);
        std::shared_ptr<EpollData> epolldata=std::make_shared<EpollData>(msock, event);
        //EpollData* epolldata = new EpollData(msock, event);
        if(ModFd(msock, event, epolldata)) {
            int mfd=msock->getsock();
            auto iter = m_events.find(mfd);
            if(iter == m_events.end()) {
                LogManager::GetInstance()->log("WebServerLog", LogLevel::WARN, "Epoll.cpp", 77, "ModFd() fail: fd does't exist", LogLevel::WARN);
                return false;
            }
            iter->second = epolldata;
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "Epoll.cpp", 81, "ModFd() success", LogLevel::INFO);
            return true;
        }    
        LogManager::GetInstance()->log("WebServerLog", LogLevel::ERROR, "Epoll.cpp", 84, "ModFd() fail: "+std::to_string(errno), LogLevel::ERROR);
        return false;
    }
    
    bool Epoll::ModFd(int mfd, uint32_t event) {
        epoll_event e;
        e.events=event;
        e.data.fd=mfd;
        int result=epoll_ctl(m_epoll_fd, EPOLL_CTL_MOD, mfd, &e);
        if(result == 0) return true;
        else std::cout<<"Epoll::ModFd() error: epoll_ctl return "<<result<<std::endl;
        return false;
    }

    bool Epoll::DelFd(Socket::ptr msock) {
        bool result = DelFd(msock->getsock());
        if(result) {
            int mfd=msock->getsock();
            auto iter = m_events.find(mfd);
            if(iter == m_events.end()) {
                LogManager::GetInstance()->log("WebServerLog", LogLevel::WARN, "Epoll.cpp", 104, "DelFd() fail: fd does't exist", LogLevel::WARN);
                return false;
            }
            m_events.erase(iter);
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "Epoll.cpp", 108, "DelFd() success", LogLevel::INFO);
            return true;
        }
        LogManager::GetInstance()->log("WebServerLog", LogLevel::ERROR, "Epoll.cpp", 111, "DelFd() fail: "+std::to_string(errno), LogLevel::ERROR);
        return false;
    }
    
    bool Epoll::DelFd(int mfd) {//要求Linux>2.6.9
        return 0==epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, mfd, NULL);  
    }

    int Epoll::Wait(epoll_event* events, int maxevents, int timeout) {
        return epoll_wait(m_epoll_fd, events, maxevents, timeout);
    }

    bool Epoll::WaitForAll(std::vector<epoll_event>& events, int timeout) {
        events.clear();
        epoll_event epollevents[256];
        for(int i=0; i<10; ++i) {
            int num = epoll_wait(m_epoll_fd, epollevents, 256, timeout/10);
            for(int j=0; j<num; ++j) events.push_back(epollevents[j]);
        }
        return events.size();                
    }
}