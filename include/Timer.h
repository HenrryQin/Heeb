#pragma once 
#ifndef _TIMER_H_
#define _TIMER_H_
#include <sys/signal.h>
#include <chrono>
#include <functional>
#include <queue>
#include <mutex>
#include "Socket.h"
#include "Thread.h"
#include "Reactor.h"

namespace Heeb {
    typedef std::chrono::steady_clock::time_point steady_time_point;
    //时间节点
    struct TimeNode {
        TimeNode(Socket::ptr sock, std::function<void(TimeNode*)> fun): 
                                        m_sock(sock), m_time(m_sock->getCloseTime()), m_handle(fun) { }
        Socket::ptr m_sock;
        steady_time_point m_time;
        std::function<void(TimeNode*)> m_handle;
    };
    
    //TimeNode比较类，作为Timer堆的参数
    class TimeNodeGreater {
    public:
        bool operator() (const TimeNode& t1, const TimeNode& t2) {
            return t1.m_time > t2.m_time;
        }
    };

    //Timer类：管理超时连接
    class Timer {
    public:
        static Timer* GetInstance(); 
        
        void init(EpollReactor& eReactor, int alarmTime=1);
        void addTimeNode(TimeNode& node);
        void addTimeNodeForHandle(TimeNode& node);
        void runTimer();
        void stopTimer();
        void tick();
        
    private:
        Timer();
        void setnonblock(int fd);
        bool setSigHandle(int sig, void(handler)(int));
        static void sigHandle(int sig);

        static Timer* m_timer;
        static int pipefd[2];
        std::priority_queue<TimeNode, std::vector<TimeNode>, TimeNodeGreater> m_time_heap;
        std::mutex heap_mutex;
        int alarmTimeSpan;
        bool isValid;
        bool isRun;
        
    };
}


#endif