#include "../include/Log.h"

namespace Heeb
{
    void StdoutAppender::log(LogLevel::level level, LogEvent::ptr eventPtr, LogLayout::ptr layout) {
        if(level>=this->m_level) std::cout<<layout->format(eventPtr);
    }

    bool FileAppender::openFile() {
        if(ofs.is_open()) ofs.close();
        ofs.open(this->filePath, std::ofstream::out | std::ofstream::app);
        return ofs.is_open();
    }
    bool FileAppender::closeFile() {
        if(ofs.is_open()) ofs.close();
        return !ofs.is_open();
    }
    void FileAppender::log(LogLevel::level level, LogEvent::ptr eventPtr, LogLayout::ptr layout) {
        if(level>=this->m_level) {
            if(!this->openFile()) {
                std::cout<<"open AppenderFile failed!"<<std::to_string(errno)<<std::endl;
                return;
            }
            layout->format(eventPtr, ofs);
        }
    }
} // namespace Hee
