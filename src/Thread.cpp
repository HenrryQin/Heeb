#include "../include/Thread.h"

namespace Heeb {
    thread_local Thread* ThisThread::thisThread = nullptr;
    
    //Thread实现
    bool Thread::joinable() {
        return m_thread.joinable();
    } 

    void Thread::join() {
        m_thread.join();
    }

    const std::string& Thread::get_threadName() {return m_name;}
    const std::thread::id Thread::get_id() {return m_thread.get_id();}
    clock_t Thread::get_elapse() {
        clock_t now=clock();
        return now-initClock;
    }
    void Thread::init(Thread* _thisThread) {
        ThisThread::thisThread = _thisThread;
        _thisThread->m_fun();
    }

    //ThisThread实现
    const std::string& ThisThread::GetName() {
        if(!thisThread) {
            std::cout<<"thisThread Error:GetName"<<std::endl;
            return "";
        }
        return thisThread->get_threadName();
    }
    const std::thread::id ThisThread::GetId() {
        if(!thisThread) {
            std::cout<<"thisThread Error:GetID"<<std::endl;
            std::thread::id idZero{};
            return idZero;
        }
        return thisThread->get_id();
    }
    clock_t ThisThread::GetElapse() {
        if(!thisThread) {
            std::cout<<"thisThread Error:GetElapse"<<std::endl;
            return 0;
        }
        return thisThread->get_elapse();
    }   

}
