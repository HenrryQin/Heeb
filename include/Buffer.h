#pragma once
#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <vector>
#include <atomic>
#include <string>
#include <algorithm>
#include <exception>
#include <iostream>
#include <sys/unistd.h>

using std::vector;
using std::atomic;
using std::size_t;
using std::string;
using std::cout;
using std::endl;

namespace Heeb {
    //Buffer类
    class Buffer {
    public:
        Buffer():m_buffer(1024), m_readPos(0), m_writePos(0) { }
        //获取参数
        size_t capacity() {return m_buffer.size();}
        size_t writableSize() {return m_buffer.size()-m_writePos;}
        size_t readableSize() {return m_writePos-m_readPos;}
        //读写位置
        char* writePosPtr() {return &m_buffer[m_writePos];}
        const char* writePosPtrConst() {return &m_buffer[m_writePos];}
        const char* readPosPtrConst() {return &m_buffer[m_readPos];}
        size_t writePos() {return m_readPos;}
        size_t readPos() {return m_writePos;}
        //读写Buff
        bool writeBuff(const string& str);
        size_t writeBuff(const char* str, size_t length);
        bool readBuff(string& str, int length=-1);
        size_t readBuff(char* str, size_t length);
        //从Buff读出至string
        void readLine(string& line);
        bool readHttpLine(string& httpLine);
        bool readHttpBody(string& httpBody, size_t length);
        //从fd读出数据至Buff，从Buff读出数据写入至fd
        ssize_t writeFd(int fd);
        ssize_t readFd(int fd);
        

    private:
        vector<char> m_buffer;
        size_t m_readPos;
        size_t m_writePos;
        
        //读位置置0、可写空间扩展
        void makeReadPosZero();
        void expand();
    };
}

#endif
