#pragma once
#ifndef _EPOLL_H_
#define _EPOLL_H_

#include <sys/epoll.h>
#include <iostream>
#include <vector>
#include <exception>
#include <stdexcept>
#include <string>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <memory>
#include "Socket.h"
#include "Log.h"

namespace Heeb {
    //epoll_event内的epoll_data
    //统一事件源为socket
    struct EpollData {
        typedef std::shared_ptr<EpollData> ptr;
        EpollData(Socket::ptr msock, uint32_t mstatus):sock(msock), status(mstatus) { }
        //int fd;
        Socket::ptr sock;
        uint32_t status;
    };

    //epoll封装
    class Epoll {
    public:
        typedef std::shared_ptr<Epoll> ptr;
        Epoll();
        bool AddFd(Socket::ptr msock, uint32_t event, EpollData::ptr eventData);
        bool AddFd(Socket::ptr msock, uint32_t event);
        bool AddFd(int mfd, uint32_t event);
        bool ModFd(Socket::ptr msock, uint32_t event, EpollData::ptr eventData);
        bool ModFd(Socket::ptr msock, uint32_t event);
        bool ModFd(int mfd, uint32_t event);
        bool DelFd(Socket::ptr msock);//要求Linux>2.6.9
        bool DelFd(int mfd);
        int Wait(epoll_event* events, int maxevents, int timeout);
        bool WaitForAll(std::vector<epoll_event>& events, int timeout);

    private:
        int m_epoll_fd;
        std::atomic<bool> is_valid;
        std::unordered_map<int, EpollData::ptr> m_events;
        std::mutex m_mapMutex;
    };
  
}

#endif