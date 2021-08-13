#include "../include/Log.h"

namespace Heeb
{
    //增删输出地
    void Logger::addAppender(LogAppender::ptr appender) {
        this->m_appenders.push_back(appender);
    }
    void Logger::delAppender(LogAppender::ptr appender) {
        for(auto appIter=this->m_appenders.begin();appIter != this->m_appenders.end(); ++appIter) {
            if(*appIter==appender) appIter=this->m_appenders.erase(appIter);
        }
    }
    void Logger::delAppender(unsigned int AppType) {
        for(auto appIter=this->m_appenders.begin();appIter != this->m_appenders.end(); ++appIter) {
            if((*appIter)->GetType()==AppType) appIter=this->m_appenders.erase(appIter);
        }
    }
    void Logger::delAppender(unsigned int AppType, const std::string& path) {
        for(auto appIter=this->m_appenders.begin();appIter != this->m_appenders.end(); ++appIter) {
            if((*appIter)->GetType()==AppType && (*appIter)->GetPath()==path) appIter=this->m_appenders.erase(appIter);
        }
    }
    //日志记录函数
    bool Logger::log(LogLevel::level log_level, LogEvent::ptr log_event) {
        if(log_level>=this->m_level) {
            for(auto& appender:this->m_appenders) {
                appender->log(log_level, log_event, this->m_LogLayout);
            }
        }
        return true;
    }
    bool Logger::logUnknown(LogEvent::ptr log_event) {
        return this->log(LogLevel::level::UNKNOW, log_event);
    }
    bool Logger::logDebug(LogEvent::ptr log_event) {
        return this->log(LogLevel::level::DEBUG, log_event);
    }
    bool Logger::logInfo(LogEvent::ptr log_event) {
        return this->log(LogLevel::level::INFO, log_event);
    }
    bool Logger::logWarn(LogEvent::ptr log_event) {
        return this->log(LogLevel::level::WARN, log_event);
    }
    bool Logger::logError(LogEvent::ptr log_event) {
        return this->log(LogLevel::level::ERROR, log_event);
    }
    bool Logger::logFatal(LogEvent::ptr log_event) {
        return this->log(LogLevel::level::FATAL, log_event);
    }
    
} // namespace Heeb
