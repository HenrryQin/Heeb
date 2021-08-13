#include "../include/WebServer.h"
#include <unistd.h>

namespace Heeb {
    WebServer* WebServer::m_instance =new WebServer();
    WebServer* WebServer::GetInstance() {
        return m_instance;
    }
    void subThreadFun() {
        WebServer::GetInstance()->sub_Reactor.AddHanle(EPOLLIN, processFun);
        WebServer::GetInstance()->sub_Reactor.AddHanle(EPOLLRDHUP | EPOLLHUP | EPOLLERR, closeFun);
        WebServer::GetInstance()->sub_Reactor.AddHanle(EPOLLOUT, sendFun);
        WebServer::GetInstance()->sub_Reactor.RunInThreadPool(WebServer::GetInstance()->m_thread_pool);
    }

    bool mainReactorFun(EpollData* data) {
        if(data->sock!=WebServer::GetInstance()->listen_Socket) {
            char sig;
            data->sock->ReceiveFd((char*)&sig, 1);
            int sign=sig;
            if(sign==SIGALRM) {
                LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 18, std::to_string(sig)+" timeTick()", LogLevel::INFO);
                Timer::GetInstance()->tick();
            }
            
            return true;
        }
        std::shared_ptr<Socket> conSocket=data->sock->Accetp();
        if(conSocket->getStatus()==Socket::SOCKET_STATUS::CONNECT) {
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 21, std::to_string(conSocket->getsock())+" connected", LogLevel::INFO);
            TimeNode node(conSocket, std::function<void(TimeNode*)>(TimeNodeHanle));
            Timer::GetInstance()->addTimeNode(node);
            
            std::shared_ptr<HttpCon> http_con=std::make_shared<HttpCon>(WebServer::GetInstance()->getResrcPath(), conSocket);
            WebServer::GetInstance()->connectMap[conSocket]=http_con;
            WebServer::GetInstance()->sub_Reactor.AddFd(conSocket, EPOLLIN|EPOLLRDHUP|EPOLLET);
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 37, "mainReactorFun() return true", LogLevel::INFO);
            return true;
        }
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 40, "mainReactorFun() return false", LogLevel::INFO);
        return false;

    }
    
    bool processFun(EpollData* data) {
        auto iter=WebServer::GetInstance()->connectMap.find(data->sock);
        if(iter==WebServer::GetInstance()->connectMap.end()) return false;
        HttpCon::ptr http_con = WebServer::GetInstance()->connectMap[data->sock];
        ssize_t size = http_con->recv();
        while(size>0) size=http_con->recv();//ET
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 39, "Receive "+std::to_string(size)+" data", LogLevel::INFO);
        if(size>0) {
            http_con->process();
            if(http_con->getStatus()==HttpCon::MAKE_RESPONSE || http_con->getStatus()==HttpCon::ERROR_REQUEST) {
                WebServer::GetInstance()->sub_Reactor.ModFd(data->sock, EPOLLIN|EPOLLRDHUP|EPOLLOUT|EPOLLET);
            }
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 54, "ProcessFun() Return true", LogLevel::INFO);           
            return true;
        }
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 57, "ProcessFun() Return false", LogLevel::INFO); 
        return false;
    }

    bool sendFun(EpollData* data) {
        HttpCon::ptr http_con = WebServer::GetInstance()->connectMap[data->sock];
        while(http_con->writeBufferSize()>0) {//ET
            ssize_t size = http_con->send();
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 61, std::to_string(size)+" bytes sent", LogLevel::INFO);
        } 
        if(http_con->getFile()) {
            int len=0;
            int size=http_con->getFileSize();
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 65, std::to_string(size)+" file to sent", LogLevel::INFO);
            while(len<size) {
                ssize_t sentsize=data->sock->SendFd(http_con->getFile()+len, size-len);
                LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 69, std::to_string(sentsize)+"bytes file sented", LogLevel::INFO);
                len += sentsize;
            }
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 72, "file sent", LogLevel::INFO);
        }
        WebServer::GetInstance()->sub_Reactor.ModFd(data->sock, EPOLLIN|EPOLLRDHUP|EPOLLET);            
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 77, "exit sendFun()", LogLevel::INFO);
        return true;    
    }

    bool closeFun(EpollData* data) {
        WebServer::GetInstance()->sub_Reactor.DelFd(data->sock);
        WebServer::GetInstance()->sub_Reactor.EraseTaskMap(data->sock);//从任务map中删除该sock
        WebServer::GetInstance()->connectMap.erase(data->sock);
        data->sock->Close();
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 80, std::to_string(data->sock->getsock())+" closed", LogLevel::INFO);
        return true;
    }

    void TimeNodeHanle(TimeNode* node) {
        auto iter=WebServer::GetInstance()->connectMap.find(node->m_sock);
        if(iter!=WebServer::GetInstance()->connectMap.end()) {//若找不到连接说明该连接已经被关闭
            if(!node->m_sock->isTimeOut()) {
                node->m_time=node->m_sock->getCloseTime();
                Timer::GetInstance()->addTimeNodeForHandle(*node);
            }
            else {
                WebServer::GetInstance()->sub_Reactor.DelFd(node->m_sock);  //移除监听描述符
                WebServer::GetInstance()->connectMap.erase(iter);   //移除http链接
                LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 84, std::to_string(node->m_sock->getsock())+" TimeOut Closed", LogLevel::INFO);
                node->m_sock->Close();  //关闭连接
            }
        }
        
    }

    WebServer::WebServer():isRun(false), subThread("subThread") {   }

    WebServer::~WebServer() {
        if(m_thread_pool) delete m_thread_pool;
        Timer::GetInstance()->stopTimer();
    }

    void WebServer::init(int threadNum, string& resrcPath, uint16_t m_port, int maxQueue, string& mlogpath) {
        logPath=mlogpath;
        LogManager::GetInstance()->addLogger("WebServerLog", LogLevel::DEBUG);
        LogManager::GetInstance()->addFileAppender("WebServerLog", mlogpath+"WebServerLog.log", LogLevel::DEBUG);
        
        m_thread_pool=new ThreadPool(threadNum, "HandleThread");
        if(resrcPath=="") {
            char path[100];
            int len = readlink("/proc/self/exe", path, 100);
            while(len>=0 && path[len]!='/') --len;
            resrc_path=string(path, len+1);
            resrc_path+"resource";
        }
        else resrc_path=resrcPath;
        listen_Socket=CreateTCPSocket(std::make_shared<Address>(IPv4, m_port));
        
        struct linger tmp={1,1};
        listen_Socket->SetSockOpt(SOL_SOCKET, SO_LINGER, tmp);
        int flag=1;
        listen_Socket->SetSockOpt(SOL_SOCKET, SO_REUSEADDR, flag);
        
        listen_Socket->Bind();
        listen_Socket->Listen(maxQueue);

        if(listen_Socket->getStatus()==Socket::LISTEN) isRun=true;
        else LogManager::GetInstance()->log("WebServerLog", LogLevel::ERROR, "WebServer.cpp", 121, "WebServer::initFailed", LogLevel::ERROR);
        
        main_Reactor.AddHanle(EPOLLIN, mainReactorFun);
        subThread.run(subThreadFun);
        
        Timer::GetInstance()->init(main_Reactor);
        Timer::GetInstance()->runTimer();
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 127, "WebServer::initSuccess", LogLevel::INFO);
    }

    void WebServer::run() {
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 124, "WebServer::run()", LogLevel::INFO);
        main_Reactor.AddFd(listen_Socket, EPOLLIN|EPOLLET);
        main_Reactor.Run();      
    }

    const string& WebServer::getResrcPath() {return resrc_path;}
    
/********************************************无ReactorL类webserver************************************************/
    bool Accept(EpollData* data) {
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 163, "enter Accept(EpollData*)", LogLevel::INFO);
        std::shared_ptr<Socket> conSocket=data->sock->Accetp();
        if(conSocket->getStatus()==Socket::SOCKET_STATUS::CONNECT) {
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 167, std::to_string(conSocket->getsock())+" connected", LogLevel::INFO);
            if(!conSocket->SetSockNonBlock()) {
                LogManager::GetInstance()->log("WebServerLog", LogLevel::ERROR, "WebServer.cpp", 169, std::to_string(conSocket->getsock())+"SetSockNonBlock() & Accept(EpollData*) return false", LogLevel::ERROR);
                return false;
            }
            TimeNode node(conSocket, std::function<void(TimeNode*)>(TimeNodeHanle));
            Timer::GetInstance()->addTimeNode(node);            
            std::shared_ptr<HttpCon> http_con=std::make_shared<HttpCon>(webserver::GetInstance()->getResrcPath(), conSocket);
            webserver::GetInstance()->connectMap[conSocket]=http_con;
            webserver::GetInstance()->web_epoll.AddFd(conSocket, EPOLLIN|EPOLLET|EPOLLRDHUP);
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 177, "Accept(EpollData*) return true", LogLevel::INFO);
            return true;
        }
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 180, "Accept(EpollData*) return false", LogLevel::INFO);
        return false;
    }

    bool Close(EpollData* data) {
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 185, "enter Close(EpollData*)", LogLevel::INFO);
        webserver::GetInstance()->web_epoll.DelFd(data->sock);
        webserver::GetInstance()->connectMap.erase(data->sock);
        data->sock->Close();
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 189, "exit Close(EpollData*)", LogLevel::INFO);
        return true;
    }

    bool Process(EpollData* data) {
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 193, "enter Process(EpollData*)", LogLevel::INFO);
        auto iter=webserver::GetInstance()->connectMap.find(data->sock);
        if(iter==webserver::GetInstance()->connectMap.end()) return false;
        HttpCon::ptr http_con = iter->second;
        ssize_t size = http_con->recv();
        while(size>0) {//ET
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 199, "Receive "+std::to_string(size)+" data", LogLevel::INFO);
            size=http_con->recv();
        }
        if(http_con->readBufferSize()>0) {
            http_con->process();
            if(http_con->getStatus()==HttpCon::MAKE_RESPONSE || http_con->getStatus()==HttpCon::ERROR_REQUEST) {
                webserver::GetInstance()->web_epoll.ModFd(data->sock, EPOLLIN|EPOLLRDHUP|EPOLLOUT|EPOLLET);
            }
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 208, "Process() Return true", LogLevel::INFO);           
            return true;
        }
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 211, "Process() Return false", LogLevel::INFO); 
        return false;
    }

    bool Send(EpollData* data) {
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 216, "enter Send(EpollData*)", LogLevel::INFO);
        HttpCon::ptr http_con = webserver::GetInstance()->connectMap[data->sock];
        while(http_con->writeBufferSize()>0) {//ET
            ssize_t size = http_con->send();
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 220, std::to_string(size)+" bytes sent", LogLevel::INFO);
        } 
        if(http_con->getFile()) {
            int len=0;
            int size=http_con->getFileSize();
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 225, std::to_string(size)+" bytes file to sent", LogLevel::INFO);
            while(len<size) {
                ssize_t sentsize=data->sock->SendFd(http_con->getFile()+len, size-len);
                LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 228, std::to_string(sentsize)+" bytes file sented", LogLevel::INFO);
                len += sentsize;
            }
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 230, "file sent", LogLevel::INFO);
        }
        webserver::GetInstance()->web_epoll.ModFd(data->sock, EPOLLIN|EPOLLRDHUP|EPOLLET);            
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 234, "exit Send(EpollData*)", LogLevel::INFO);
        return true;    
    }

    webserver::webserver():isRun(false) {

    }

    webserver* webserver::m_instance = new webserver();
    webserver* webserver::GetInstance() {
        return m_instance;
    }
    void webserver::init(int threadNum, string& resrcPath, uint16_t m_port, int maxQueue, string& mlogpath) {
        
        LogManager::GetInstance()->addLogger("WebServerLog", LogLevel::DEBUG);
        LogManager::GetInstance()->addFileAppender("WebServerLog", mlogpath+"WebServerLog.log", LogLevel::DEBUG);
        
        logPath=mlogpath;
        m_thread_pool=new ThreadPool(threadNum, "HandleThread");
        if(resrcPath=="") {
            char path[100];
            int len = readlink("/proc/self/exe", path, 100);
            while(len>=0 && path[len]!='/') --len;
            resrc_path=string(path, len+1);
            resrc_path+"resource";
        }
        else resrc_path=resrcPath;
        listen_Socket=CreateTCPSocket(std::make_shared<Address>(IPv4, m_port));
        
        struct linger tmp={1,1};
        listen_Socket->SetSockOpt(SOL_SOCKET, SO_LINGER, tmp);
        int flag=1;
        listen_Socket->SetSockOpt(SOL_SOCKET, SO_REUSEADDR, flag);
        
        listen_Socket->Bind();
        listen_Socket->Listen(maxQueue);

        if(listen_Socket->getStatus()==Socket::LISTEN) isRun=true;
        else LogManager::GetInstance()->log("WebServerLog", LogLevel::ERROR, "WebServer.cpp", 272, "webserver::initFailed", LogLevel::ERROR);
        
        //Timer::GetInstance()->init(main_Reactor);
        //Timer::GetInstance()->runTimer();

        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 277, "webserver::initSuccess", LogLevel::INFO);
    }

    void webserver::run() {
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 281, "eunter webserver::run()", LogLevel::INFO);
        web_epoll.AddFd(listen_Socket, EPOLLIN);
        while(isRun) {
            epoll_event events[128];
            int size = web_epoll.Wait(events, 128, 1000);
            for(int i=0; i<size; ++i) {
                uint32_t event = events[i].events;
                EpollData* data = (EpollData*) events[i].data.ptr;

                LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 290, std::to_string(event)+" cap", LogLevel::INFO);
                
                if(data->sock==listen_Socket) Accept(data);
                else {
                    auto iter=futureMap.find(data->sock);
                    if(iter!=futureMap.end()) iter->second.wait();

                    if(event & (EPOLLHUP|EPOLLRDHUP|EPOLLERR)) {
                        Close(data);
                        futureMap.erase(data->sock);
                    }
                    else if(event & EPOLLIN) futureMap[data->sock] = m_thread_pool->addTask(Process, data);
                    else if(event & EPOLLOUT) futureMap[data->sock] = m_thread_pool->addTask(Send, data);
                }

                LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "WebServer.cpp", 305, std::to_string(event)+" handle", LogLevel::INFO);
            }


        }
    }

    void webserver::stop() {
        isRun=false;
    }

    const string& webserver::getResrcPath() {return resrc_path;}
}