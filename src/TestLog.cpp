#include "../include/Log.h"
using namespace std;
using namespace Heeb;
#define TESTLOG 0

#if TESTLOG
void thread_f1(int i) {
    LogManager::GetInstance()->log("Stdout", LogLevel::UNKNOW, "TestLog.cpp", 7, "Stdout->"+to_string(i), (LogLevel::level) (i%6));
    LogManager::GetInstance()->log("Stdout", LogLevel::DEBUG, "TestLog.cpp", 8, "Stdout->"+to_string(i), (LogLevel::level) (i%6));
    LogManager::GetInstance()->log("Stdout", LogLevel::INFO, "TestLog.cpp", 9, "Stdout->"+to_string(i), (LogLevel::level) (i%6));
    LogManager::GetInstance()->log("Stdout", LogLevel::WARN, "TestLog.cpp", 10, "Stdout->"+to_string(i), (LogLevel::level) (i%6));
    LogManager::GetInstance()->log("Stdout", LogLevel::ERROR, "TestLog.cpp", 11, "Stdout->"+to_string(i), (LogLevel::level) (i%6));
    LogManager::GetInstance()->log("Stdout", LogLevel::FATAL, "TestLog.cpp", 12, "Stdout->"+to_string(i), (LogLevel::level) (i%6));
}

void thread_f2(int i) {
    LogManager::GetInstance()->log("LogFile", LogLevel::UNKNOW, "TestLog.cpp", 16, "Stdout->"+to_string(i), (LogLevel::level) (i%6));
    LogManager::GetInstance()->log("LogFile", LogLevel::DEBUG, "TestLog.cpp", 17, "Stdout->"+to_string(i), (LogLevel::level) (i%6));
    LogManager::GetInstance()->log("LogFile", LogLevel::INFO, "TestLog.cpp", 18, "Stdout->"+to_string(i), (LogLevel::level) (i%6));
    LogManager::GetInstance()->log("LogFile", LogLevel::WARN, "TestLog.cpp", 19, "Stdout->"+to_string(i), (LogLevel::level) (i%6));
    LogManager::GetInstance()->log("LogFile", LogLevel::ERROR, "TestLog.cpp", 20, "Stdout->"+to_string(i), (LogLevel::level) (i%6));
    LogManager::GetInstance()->log("LogFile", LogLevel::FATAL, "TestLog.cpp", 21, "Stdout->"+to_string(i), (LogLevel::level) (i%6));
}

int main() {   
    LogManager::GetInstance()->addLogger("Stdout", LogLevel::UNKNOW, "%d{%Y-%m-%d %H:%M:%S} [%r] %c[%p] %t[%N] %f[%l] %m%n");
    LogManager::GetInstance()->addLogger("LogFile", LogLevel::DEBUG, "%d{%Y-%m-%d %H:%M:%S}%T[%r] %c[%p] %t[%N] %f[%l] %m%n");
    LogManager::GetInstance()->addStdoutAppender("Stdout", LogLevel::DEBUG);
    LogManager::GetInstance()->addStdoutAppender("Log1", LogLevel::DEBUG);
    LogManager::GetInstance()->addFileAppender("LogFile", "/home/qin_haoyu/HeebLog/Debug.log", LogLevel::DEBUG);
    LogManager::GetInstance()->addFileAppender("LogFile", "/home/qin_haoyu/HeebLog/Info.log", LogLevel::INFO);
    LogManager::GetInstance()->addFileAppender("LogFile", "/home/qin_haoyu/HeebLog/Warn.log", LogLevel::WARN);
    for(int i=0; i<10; ++i) {
        Thread::ptr t1 = std::make_shared<Thread>("ThreadTestLog1");
        Thread::ptr t2 = std::make_shared<Thread>("ThreadTestLog2");
        t1->run(thread_f1, i);
        t2->run(thread_f2, i);
        t1->join();
        t2->join();
    }
}

#endif