#pragma once
#ifndef _LOG_H_
#define _LOG_H_

#include <memory>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <fstream>
#include <mutex>
#include <thread>
#include "Thread.h"

//appender标志位
#define APP_BASE 1
#define APP_STDOUT 2
#define APP_FILE 4

namespace Heeb {
    class LogLevel {
    public:
        enum level {
            UNKNOW  =   0,
            DEBUG   =   1,
            INFO    =   2,
            WARN    =   3,
            ERROR   =   4,
            FATAL   =   5
        };
    };
    class Logger;
    //日志事件
    class LogEvent {
    public:
        typedef std::shared_ptr<LogEvent> ptr;
        LogEvent(std::string fileName, uint32_t line, std::string content, LogLevel::level Level, 
                time_t time, clock_t elapse, std::thread::id thread_id, std::string thread_name,  std::shared_ptr<Logger> logger) :
                m_filename(fileName), m_line(line), m_content(content), m_level(Level), m_time(time), m_elapse(elapse), 
                m_thread_id(thread_id), m_thread_name(thread_name), m_logger(logger) { }

        const std::string& GetName() {return m_filename;}
        uint32_t GetLine() {return m_line;}
        const std::string& GetContent() {return m_content;}
        LogLevel::level GetLevel() {return m_level;}
        time_t GetTime() {return m_time;}
        clock_t GetElapse() {return m_elapse;}
        std::thread::id GetThreadId() {return m_thread_id;}
        const std::string& GetThreadName() {return m_thread_name;}  
        std::shared_ptr<Logger> GetLogger() {return m_logger;}       
    private:
        std::string m_filename;     //名称
        uint32_t m_line;            //行号
        std::string m_content;      //内容
        LogLevel::level m_level;    //等级
        time_t m_time;              //时间戳
        clock_t m_elapse;          //软件启动时间
        std::thread::id m_thread_id; //线程id
        std::string m_thread_name;  //线程名称
        std::shared_ptr<Logger> m_logger;       //日志器

    };

    //日志格式化器
    class LogLayout {
    public:
        typedef std::shared_ptr<LogLayout> ptr;

        class LayoutItem {
        public:
            typedef std::shared_ptr<LayoutItem> ptr;
            virtual void layout(std::ostream& os, LogEvent::ptr eventPtr)=0;
            virtual ~LayoutItem()=default;
        };

        //构造函数
        //%d 输出日志时间点的日期或时间，默认格式为ISO8601，也可以在其后指定格式，比如：%d{%Y-%m-%d %H:%M:%S}，输出类似：2002-10-18 22：10：28
        //%r 输出自应用启动到输出该日志信息所耗费的毫秒数
        //%c 输出日志信息的日志器名称
        //%p 输出优先级，即DEBUG，INFO，WARN，ERROR，FATAL。如果是调用debug()输出的，则为[DEBUG]，依此类推
        //%t 输出日志事件中线程id
        //%N 输出日志线程名
        //%f 输出日志信息所属文件名
        //%l 输出日志事件的发生位置（行号）
        //%m 输出代码中指定的信息，如log(message)中的message :LogEvent中的m_content
        //%T 输出制表位
        //%n 输出一个回车换行符
        //2002-10-18 22：10：28 [10246]    root[DEBUG] 2103[main] server.cpp[52] sometingLogged...
        LogLayout(std::string pattern="%d{%Y-%m-%d %H:%M:%S} [%r] %c[%p] %t[%N]%T%f[%l] %m%n") : m_pattern(pattern) {}
        //初始化，将pattern翻译成formatters
        bool Init();
        //改变日志模式
        bool chageLayout(std::string newPattern) {
            this->m_pattern = newPattern;
            this->Init();
            return true;
        }
        //格式化日志事件至输出流或string
        void format(LogEvent::ptr eventPtr, std::ostream& os);
        std::string format(LogEvent::ptr eventPtr);


    private:
        std::string m_pattern;
        std::vector<LayoutItem::ptr> m_LayoutItems;
    };

    //日志输出地：基类
    class LogAppender {
    public:
        typedef std::shared_ptr<LogAppender> ptr;
        LogAppender(LogLevel::level l = LogLevel::UNKNOW):m_level(l) { }
        virtual ~LogAppender() = default;
        
        virtual void log(LogLevel::level level, LogEvent::ptr eventPtr, LogLayout::ptr layout) = 0;
        
        unsigned int GetType() {
            return AppenderType;
        }
        //标志路径,设计该函数返回值为某一类型的某个appender的标志名
        virtual const std::string& GetPath() = 0;
    protected:
        LogLevel::level m_level;
        unsigned int AppenderType = APP_BASE;

    };

    class StdoutAppender : public LogAppender {
    public:
        StdoutAppender(LogLevel::level l = LogLevel::UNKNOW) {
            m_level = l;
            AppenderType = APP_STDOUT;
        }
        const std::string& GetPath() override {return path;}
        void log(LogLevel::level level, LogEvent::ptr eventPtr, LogLayout::ptr layout) override;
    private:
        const std::string path="Stdout";

    };

    class FileAppender : public LogAppender {
    public:
        FileAppender(std::string path, LogLevel::level l=LogLevel::UNKNOW): filePath(path) {
            m_level=l;
            AppenderType = APP_FILE;
            std::string fileName;
            for(int i=1; i<path.size(); ++i) {
                if(path[i]=='/') {
                    std::string subPath=path.substr(0, i+1);
                    if(access(subPath.c_str(), F_OK)) {
                        if(-1 != mkdir(subPath.c_str(), S_IRWXU | S_IRWXG | S_IRWXO)) {
                            std::cout<<"File Path mkdir Failed"<<std::endl;
                            break;
                        }
                    }
                } 
            }
        }
        const std::string& GetPath() override {return filePath;}
        void log(LogLevel::level level, LogEvent::ptr eventPtr, LogLayout::ptr layout) override;
        //打开或关闭文件，成功返回true
        bool openFile();
        bool closeFile();
    private:
        std::string filePath;
        std::ofstream ofs;

    };

    //日志器
    class Logger {
    public:
        typedef std::shared_ptr<Logger> ptr;
        //构造函数
        Logger(std::string name="root", LogLevel::level level=LogLevel::UNKNOW, std::string pattern="%d{%Y-%m-%d %H:%M:%S} [%r] %c[%p] %t[%N]%T%f[%l] %m%n")
                :m_name(name), m_level(level), m_LogLayout(new LogLayout(pattern)) {
            this->m_LogLayout->Init();//构造时格式初始化
        }
        //增删输出地
        void addAppender(LogAppender::ptr appender);
        void delAppender(LogAppender::ptr appender);
        void delAppender(unsigned int AppType);//删除某一类型appender
        void delAppender(unsigned int AppType, const std::string& filepath);//删除某一类型的GetPath()为path的appender
        //日志记录函数
        bool log(LogLevel::level log_level, LogEvent::ptr log_event);
        bool logUnknown(LogEvent::ptr log_event);
        bool logDebug(LogEvent::ptr log_event);
        bool logInfo(LogEvent::ptr log_event);
        bool logWarn(LogEvent::ptr log_event);
        bool logError(LogEvent::ptr log_event);
        bool logFatal(LogEvent::ptr log_event);
        //获取日志名
        const std::string& GetName() {return m_name;}
        //改变格式
        void changeLayout(const std::string& pattern) {
            this->m_LogLayout->chageLayout(pattern);
        }
        void changeLayout(LogLayout::ptr Layout) {
            this->m_LogLayout=Layout;
        }  
    private:
        std::string m_name;
        LogLevel::level m_level;
        std::list<LogAppender::ptr> m_appenders;
        LogLayout::ptr m_LogLayout;
    };

    //日志管理单例类
    class LogManager {
    public:
        //饿汉模式
        static LogManager* GetInstance() {
            return m_LogManager;
        }
        //增删日志器
        void addLogger(const std::string& loggerName, LogLevel::level loggerLevel, const std::string& loggerFormat="%d{%Y-%m-%d %H:%M:%S} [%r] %c[%p] %t[%N] %f[%l] %m%n");
        void delLogger(const std::string& loggerName);
        //判断日志器是否存在
        bool isLoggerExisted(const std::string& loggerName);
        //为日志器增删【控制台输出】
        bool addStdoutAppender(const std::string& loggerName, LogLevel::level mlevel) ;
        bool delStdoutAppender(const std::string& loggerName);
        //为日志器增删【文件输出】
        bool addFileAppender(const std::string& loggerName, const std::string& FilePath, LogLevel::level mlevel);
        bool delFileAppender(const std::string& loggerName, const std::string& FilePath);
        //记录日志
        bool log(const std::string& loggerName, LogLevel::level log_level, const std::string& fileName, uint32_t lineNo, const std::string& content, LogLevel::level Eventlevel);

    private:
        std::unordered_map<std::string, Logger::ptr> loggerNameMap;//根据日志名称索引logger
        static LogManager* m_LogManager;
        std::mutex logMutex;//日志锁

        LogManager(); 
        LogManager(const LogManager&)=delete;
        LogManager& operator = (const LogManager&)=delete;
        ~LogManager();
    };
}




#endif