#pragma once
#ifndef _WEBSERBER_H_
#define _WEBSERBER_H_

#include "Timer.h"
#include "HttpConnection.h"
#include "Log.h"
#include "ThreadPool.h"
#include "Reactor.h"

namespace Heeb
{
    
    void subThreadFun();
    bool mainReactorFun(EpollData*);
    bool processFun(EpollData*);
    bool sendFun(EpollData*);
    bool closeFun(EpollData*);

    void TimeNodeHanle(TimeNode*);

    class WebServer {
    public:
        EpollReactor main_Reactor;
        EpollReactor sub_Reactor;
        ThreadPool* m_thread_pool;
        Socket::ptr listen_Socket;
        unordered_map<Socket::ptr, HttpCon::ptr> connectMap;

        static WebServer* GetInstance(); 
        void init(int threadNum, string& resrcPath, uint16_t m_port, int maxQueue, string& mlogpath);
        void run();
        void stop();
        const string& getResrcPath();

    private:
        WebServer();
        ~WebServer();
        static WebServer* m_instance;
        bool isRun;
        string resrc_path;
        string logPath;      
        Thread subThread;       
    };


    bool Accept(EpollData*);
    bool Close(EpollData*);
    bool Process(EpollData*);
    bool Send(EpollData*);

    class webserver {
    public:
        ThreadPool* m_thread_pool;
        Socket::ptr listen_Socket;
        unordered_map<Socket::ptr, HttpCon::ptr> connectMap;
        unordered_map<Socket::ptr, std::future<bool>> futureMap;
        Epoll web_epoll;

        static webserver* GetInstance(); 
        void init(int threadNum, string& resrcPath, uint16_t m_port, int maxQueue, string& mlogpath);
        void run();
        void stop();
        const string& getResrcPath();
    private:
        static webserver* m_instance;
        bool isRun;
        string resrc_path;
        string logPath;
        webserver();
    };

    

}




#endif