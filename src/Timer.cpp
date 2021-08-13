#include "../include/Timer.h"
#include <fcntl.h>
#include <signal.h>

namespace Heeb {
    int Timer::pipefd[2];
    //单例静态变量
    Timer* Timer::m_timer = new Timer();
    //单例函数
    Timer* Timer::GetInstance() {
        return m_timer;
    }
    //构造函数
    Timer::Timer():m_time_heap(TimeNodeGreater(), std::vector<TimeNode>()), isValid(false), isRun(false) { }
    //设置描述符非阻塞
    void Timer::setnonblock(int fd) {
        int opt=fcntl(fd, F_GETFL);
        opt=opt|O_NONBLOCK;
        fcntl(fd, F_SETFL, opt);
    }
    //初始化
    void Timer::init(EpollReactor& eReactor, int alarmTime) {
        alarmTimeSpan = alarmTime;
        if(socketpair(AF_UNIX, SOCK_STREAM, 0, pipefd)==-1) {
            cout<<"PipeFd Create Failed: errno is "<<errno<<endl;
            return;
        }
        setnonblock(pipefd[1]);
        Socket::ptr timerSock=CreatePipeSocket(pipefd[0]);
        if(!eReactor.AddFd(timerSock, EPOLLIN)) {
            cout<<"Timer: Epoll AddFd Failed"<<endl;
            return ;
        }
        setnonblock(pipefd[0]);
        if(!setSigHandle(SIGALRM, sigHandle)) {
            cout<<"Timer: SigHandle set Failed"<<endl;
            return ;
        } 
        isValid=true;
    }
    //添加节点：有锁
    void Timer::addTimeNode(TimeNode& node) {
        std::lock_guard<std::mutex> heap_lock(heap_mutex);
        m_time_heap.push(node);
    }
    //添加节点：无锁
    void Timer::addTimeNodeForHandle(TimeNode& node) {
        m_time_heap.push(node);
    }

    void Timer::tick() {
        std::lock_guard<std::mutex> heap_lock(heap_mutex);
        while(!m_time_heap.empty() && m_time_heap.top().m_time>=std::chrono::steady_clock::now()) {
            TimeNode node=m_time_heap.top();
            m_time_heap.pop();
            node.m_handle(&node);//注意:m_handle中对时间堆的操作无需加锁，否则会死锁。
            
        }
        if(isValid) alarm(alarmTimeSpan);
    }

    void Timer::runTimer() {
        if(isValid) {
            isRun=true;
            alarm(alarmTimeSpan);
        }
    }
    void Timer::stopTimer() {
        isRun=false;
    }

    void Timer::sigHandle(int sig) {
        int save_errno=errno;
        send(pipefd[1], (char*) &sig, 1, 0);
        errno=save_errno;
    }

    bool Timer::setSigHandle(int sig, void(handler)(int)) {
        struct sigaction sa;
        memset(&sa, '\0', sizeof(sa));
        sa.sa_handler = handler;
        sigfillset(&sa.sa_mask);
        return sigaction(sig, &sa, NULL)==0;

    }

}