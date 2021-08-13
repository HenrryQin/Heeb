#include "../include/Buffer.h"

namespace Heeb {
    //http行结束符
    static const string CRLF="\r\n";
    //读位置置0
    void Buffer::makeReadPosZero() {
        int i=0;
        std::copy(readPosPtrConst(), writePosPtrConst(), m_buffer.begin());
        m_writePos=readableSize();
        m_readPos=0;
    }

    void Buffer::expand() {
        if(readPos()>capacity()/2) makeReadPosZero();
        else m_buffer.resize(2*m_buffer.capacity());
    }

    //读写Buff
    bool Buffer::writeBuff(const string& str) {
        return writeBuff(str.data(), str.size())==str.size();
    }
    size_t Buffer::writeBuff(const char* str, size_t length) {
        try{
            while(writableSize()<=length) expand();
            std::copy(str, str+length, writePosPtr());
        }
        catch(std::exception& e) {
            std::cout<<"writeBuff Failed: "<<e.what()<<endl;
            return 0;
        }  
        m_writePos+=length;
        return length;
    }

    bool Buffer::readBuff(string& str, int length) {
        if(length<0 || length>readableSize()) length=readableSize();
        str.resize(length);
        return readBuff(&(*str.begin()), length)==length;
    }

    size_t Buffer::readBuff(char* str, size_t length) {
        try{
            if(length>readableSize()) length=readableSize();
            std::copy(readPosPtrConst(), readPosPtrConst()+length, str);           
        }
        catch(std::exception& e) {
            std::cout<<"readBuff Failed: "<<e.what()<<endl;
            return 0;
        }  
        m_readPos+=length;
        if(m_readPos==m_writePos) m_readPos=m_writePos=0;
        return length;
    }

    //从Buff读出至string
    void Buffer::readLine(string& line) {
        line="";
        while(m_readPos!=m_writePos && *readPosPtrConst()!='\n') {
            line+=(*readPosPtrConst());
            ++m_readPos;
        }
        if(m_readPos==m_writePos) m_readPos=m_writePos=0;
    }

    bool Buffer::readHttpLine(string& httpLine) {
        const char* httpLineEnd=std::search(readPosPtrConst(), writePosPtrConst(), CRLF.begin(), CRLF.end());
        if(httpLineEnd==writePosPtrConst()) return false;
        httpLine = string(readPosPtrConst(), httpLineEnd-readPosPtrConst()+2);
        m_readPos+=httpLine.size();
        return true;
    }

    bool Buffer::readHttpBody(string& httpBody, size_t length) {
        if(length>readableSize()) return false;
        return readBuff(httpBody, length);
    }

    //从fd读出数据至Buff，从Buff读出数据写入至fd
    ssize_t Buffer::writeFd(int fd) {
        const void* readpos=(const void*)readPosPtrConst();
        ssize_t size = write(fd, readpos, readableSize());
        if(size>0) {
            m_readPos+=size;
            if(m_readPos==m_writePos) m_readPos=m_writePos=0;
        }
        
        return size;
    }

    ssize_t Buffer::readFd(int fd) {
        while(writableSize()<4096) expand();
        int size=read(fd, (void*)writePosPtr(), 4096);
        if(size>0) m_writePos+=size;
        return size;
    }
}