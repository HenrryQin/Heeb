#include "../include/Log.h"
#include <iomanip>

namespace Heeb {
    std::string LevelToString(LogLevel::level level) {
        std::string str;
        switch (level)
        {
        case 1:
            str="debug";
            break;
        case 2:
            str="info";
            break;
        case 3:
            str="warn";
            break;
        case 4:
            str="error";
            break;
        case 5:
            str="fatal";
            break;
        default:
            str="unknow";
            break;
        }
        return str;
    }

    //filename: %f
    class nameLayoutItem:public LogLayout::LayoutItem {
    public:
        void layout(std::ostream& os, LogEvent::ptr eventPtr) override {
            os<<eventPtr->GetName();
        }
    };
    //lineNo:   %l
    class lineLayoutItem:public LogLayout::LayoutItem {
    public:
        void layout(std::ostream& os, LogEvent::ptr eventPtr) override {
            os<<eventPtr->GetLine();
        }
    };
    //content:  %m
    class contentLayoutItem:public LogLayout::LayoutItem {
    public:
        void layout(std::ostream& os, LogEvent::ptr eventPtr) override {
            os<<eventPtr->GetContent();
        }
    };
    //level:   %p
    class levelLayoutItem:public LogLayout::LayoutItem {
    public:
        void layout(std::ostream& os, LogEvent::ptr eventPtr) override {
            os<<LevelToString(eventPtr->GetLevel());
        }
    };
    //time: %d{time_pattern}
    class timeLayoutItem:public LogLayout::LayoutItem {
    public:
        timeLayoutItem(std::string pattern="%F") : time_pattern(pattern) {}

        void layout(std::ostream& os, LogEvent::ptr eventPtr) override {
            time_t event_time=eventPtr->GetTime();
            localtime_r(&event_time, &(this->m_tm));
            os<<std::put_time(&m_tm, time_pattern.c_str());
        }
    private:
        std::string time_pattern;
        tm m_tm;
    };
    //elapse:   %r
    class elapseLayoutItem:public LogLayout::LayoutItem {
    public:
        void layout(std::ostream& os, LogEvent::ptr eventPtr) override {
            os<<eventPtr->GetElapse();
        }
    };
    //threadId: %t
    class threadIdLayoutItem:public LogLayout::LayoutItem {
    public:
        void layout(std::ostream& os, LogEvent::ptr eventPtr) override {
            os<<eventPtr->GetThreadId();
        }
    };
    //threadName:   %N
    class threadNameLayoutItem:public LogLayout::LayoutItem {
    public:
        void layout(std::ostream& os, LogEvent::ptr eventPtr) override {
            os<<eventPtr->GetThreadName();
        }
    };

    //loggerName:   %c
    class loggerNameLayoutItem:public LogLayout::LayoutItem {
    public:
        void layout(std::ostream& os, LogEvent::ptr eventPtr) override {
            os<<eventPtr->GetLogger()->GetName();
        }
    };
    //Tab:  %T
    class tabLayoutItem:public LogLayout::LayoutItem {
    public:
        void layout(std::ostream& os, LogEvent::ptr eventPtr) override {
            os<<"\t";
        }
    };
    //Enter:  %n
    class enterLayoutItem:public LogLayout::LayoutItem {
    public:
        void layout(std::ostream& os, LogEvent::ptr eventPtr) override {
            os<<"\n";
        }
    };
    //string: 
    class stringLayoutItem:public LogLayout::LayoutItem {
    public:
        stringLayoutItem(const std::string& str) : m_str(str) { }
        void layout(std::ostream& os, LogEvent::ptr eventPtr) override {
            os<<m_str;
        }
    private:
        std::string m_str;
    };


    bool LogLayout::Init() {
        //“%d{%Y-%m-%d %H:%M:%S} [%r] %c[%p] %t[%N]%T%f[%l] %m%n”
        int i=0;
        while(i<this->m_pattern.size()) {
            if(this->m_pattern[i]!='%') {
                std::string str="";
                str+=this->m_pattern[i];
                ++i;
                while(i<this->m_pattern.size() && this->m_pattern[i]!='%') {
                    str+=this->m_pattern[i];
                    ++i;
                }
                this->m_LayoutItems.push_back(std::make_shared<stringLayoutItem>(str));
            }
            else {
                ++i;
                char formatChar=this->m_pattern[i];
                if(formatChar=='f') this->m_LayoutItems.push_back(std::make_shared<nameLayoutItem>());
                else if(formatChar=='l') this->m_LayoutItems.push_back(std::make_shared<lineLayoutItem>());
                else if(formatChar=='m') this->m_LayoutItems.push_back(std::make_shared<contentLayoutItem>());
                else if(formatChar=='p') this->m_LayoutItems.push_back(std::make_shared<levelLayoutItem>());
                else if(formatChar=='d') {
                    std::string pattern="";
                    if(i+1<this->m_pattern.size() && this->m_pattern[i+1]=='{') {
                        i+=2;
                        while(i<this->m_pattern.size() && this->m_pattern[i]!='}') {
                            pattern+=this->m_pattern[i];
                            ++i;
                        }
                        if(i<this->m_pattern.size()) this->m_LayoutItems.push_back(std::make_shared<timeLayoutItem>(pattern));
                        else this->m_LayoutItems.push_back(std::make_shared<stringLayoutItem>("timeFormatError"));
                    }
                    else this->m_LayoutItems.push_back(std::make_shared<timeLayoutItem>());
                }
                else if(formatChar=='r') this->m_LayoutItems.push_back(std::make_shared<elapseLayoutItem>());
                else if(formatChar=='t') this->m_LayoutItems.push_back(std::make_shared<threadIdLayoutItem>());
                else if(formatChar=='N') this->m_LayoutItems.push_back(std::make_shared<threadNameLayoutItem>());
                else if(formatChar=='c') this->m_LayoutItems.push_back(std::make_shared<loggerNameLayoutItem>());
                else if(formatChar=='T') this->m_LayoutItems.push_back(std::make_shared<tabLayoutItem>());
                else if(formatChar=='n') this->m_LayoutItems.push_back(std::make_shared<enterLayoutItem>());
                else if(formatChar=='%') this->m_LayoutItems.push_back(std::make_shared<stringLayoutItem>("%"));
                else this->m_LayoutItems.push_back(std::make_shared<stringLayoutItem>("UnknowPatternChar!"));
                ++i;
            }
        }
        return true;

    } 

    void LogLayout::format(LogEvent::ptr eventPtr, std::ostream& os) {
        for(auto& Item:this->m_LayoutItems) Item->layout(os, eventPtr);
    }

    std::string LogLayout::format(LogEvent::ptr eventPtr) {
        std::ostringstream oss;
        this->format(eventPtr, oss);
        return oss.str();
    }
}