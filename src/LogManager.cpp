#include "../include/Log.h"

namespace Heeb
{  

    LogManager* LogManager::m_LogManager = new LogManager();
    LogManager::LogManager() {
       loggerNameMap["root"] = std::make_shared<Logger>();
    }
    LogManager::~LogManager() {
        delete m_LogManager;
    }
   //增删日志器
    void LogManager::addLogger(const std::string& loggerName, LogLevel::level loggerLevel, const std::string& loggerFormat) {
        std::lock_guard<std::mutex> logLock(logMutex);//加日志锁
        auto iter=loggerNameMap.find(loggerName);
        if(iter!=loggerNameMap.end()) {
            std::cout<<"The logger " + loggerName + " is existed, created Failed!"<<std::endl;
            return;
        }
        loggerNameMap[loggerName]=std::make_shared<Logger>(loggerName, loggerLevel, loggerFormat);
    }
    void LogManager::delLogger(const std::string& loggerName){
        std::lock_guard<std::mutex> logLock(logMutex);//加日志锁
        if(0 == loggerNameMap.erase(loggerName)) std::cout<<"The logger " + loggerName + " is not existed!"<<std::endl;
    }
    //判断日志器是否存在
    bool LogManager::isLoggerExisted(const std::string& loggerName) {
        auto iter=loggerNameMap.find(loggerName);
        return iter!=loggerNameMap.end();
    }
    //为日志器增删【控制台输出】
    bool LogManager::addStdoutAppender(const std::string& loggerName, LogLevel::level mlevel) {
        std::lock_guard<std::mutex> logLock(logMutex);//加日志锁
        if(!isLoggerExisted(loggerName)) {
            std::cout<<"Logger: "<<loggerName<<" isn't existed"<<std::endl;
            return false;
        }
        loggerNameMap[loggerName]->addAppender(std::make_shared<StdoutAppender>(mlevel));
        return true;
    }
    bool LogManager::delStdoutAppender(const std::string& loggerName) {
        std::lock_guard<std::mutex> logLock(logMutex);//加日志锁 
        if(!isLoggerExisted(loggerName)) {
            std::cout<<"Logger: "<<loggerName<<" isn't existed"<<std::endl;
            return false;
        }
        loggerNameMap[loggerName]->delAppender(APP_STDOUT);
        return true;
    }
    //为日志器增删【文件输出】
    bool LogManager::addFileAppender(const std::string& loggerName, const std::string& FilePath, LogLevel::level mlevel) {
        std::lock_guard<std::mutex> logLock(logMutex);//加日志锁
        if(!isLoggerExisted(loggerName)) {
            std::cout<<"Logger: "<<loggerName<<" isn't existed"<<std::endl;
            return false;
        }
        loggerNameMap[loggerName]->addAppender(std::make_shared<FileAppender>(FilePath, mlevel));
        return true;
    }
    bool LogManager::delFileAppender(const std::string& loggerName, const std::string& FilePath) {
        std::lock_guard<std::mutex> logLock(logMutex);//加日志锁
        if(!isLoggerExisted(loggerName)) {
            std::cout<<"Logger: "<<loggerName<<" isn't existed"<<std::endl;
            return false;
        }
        loggerNameMap[loggerName]->delAppender(APP_FILE, FilePath);
        return true;
    }

    //记录日志
    bool LogManager::log(const std::string& loggerName, LogLevel::level log_level, const std::string& fileName, uint32_t lineNo, const std::string& content, LogLevel::level Eventlevel) {
        std::lock_guard<std::mutex> logLock(logMutex);//加日志锁
        if(!isLoggerExisted(loggerName)) {
            std::cout<<"Logger: "<<loggerName<<" isn't existed"<<std::endl;
            return false;
        }
        clock_t elapse=ThisThread::GetElapse();
        LogEvent::ptr event = std::make_shared<LogEvent>(fileName, lineNo, content, Eventlevel, time(0), elapse, ThisThread::GetId(), ThisThread::GetName(), loggerNameMap[loggerName]); 
        loggerNameMap[loggerName]->log(log_level, event);
        return true;
    }
    
   
} // namespace Heeb
