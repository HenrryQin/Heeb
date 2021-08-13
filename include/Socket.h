#pragma once
#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <string>
#include <memory>
#include <chrono>
#include "Buffer.h"

namespace Heeb{
    //socket类型
    enum SocketType {
        TCP=SOCK_STREAM,
        UDP=SOCK_DGRAM
    };
    //协议族类型
    enum Family {
        IPv4=AF_INET,
        IPv6=AF_INET6,
        PIPE=AF_UNIX,
    };
    //地址共用体
    union address
    {
        sockaddr_in address4;
        sockaddr_in6 address6;
    };
    

    //通信地址封装
    class Address {
    public:
        typedef std::shared_ptr<Address> ptr;
        //创建默认地址
        Address(Family netType = IPv4, uint16_t port = 80);
        //创建ipv4地址
        Address(uint32_t ip_4, uint16_t port);
        //创建ipv6地址(地址必须是网络字节序), 端口为主机字节序
        Address(in6_addr addr, uint16_t port);

        //根据字符串创建地址
        Address(Family netType, const char* ip, uint16_t port);
        Address(Family netType, const std::string& ip, uint16_t port);
        

        //根据sockaddr_in(6)构造
        Address(const sockaddr_in& sockaddr_4);
        Address(const sockaddr_in6& sockaddr_6);
        const Family& GetFamily();
        sockaddr* GetSockAddr();
        socklen_t GetSocklen();
        sockaddr_in GetSockAddrIn();

    private:
        Family m_family;    //地址类型
        address m_address;  //地址
    };

    typedef std::chrono::steady_clock::time_point steady_time_point;
    //socket封装
    class Socket {
    public:
        enum SOCKET_STATUS {
            INVALID,
            PIPE,
            INIT,
            BIND,
            LISTEN,
            CONNECT,
            CLOSE
        };
        typedef std::shared_ptr<Socket> ptr;
        //构造
        Socket(SocketType stype=TCP, Address::ptr localAddress=std::make_shared<Address>());
        Socket(SocketType stype, Address::ptr localAddress, int socketfd);
        //参数读取、设置
        bool GetSockOpt(int level, int optName, void* optVal, socklen_t* optLen);

        template<typename T>
        bool GetSockOpt(int level, int optName, T& optVal) {
            socklen_t len=sizeof(T);
            return GetSockOpt(level, optName, &optVal, &len);
        }

        bool SetSockOpt(int level, int optName, void* optVal, socklen_t optLen);

        template<typename T>
        bool SetSockOpt(int level, int optName, T& optVal) {
            socklen_t len=sizeof(T);
            return SetSockOpt(level, optName, &optVal, len);
        }
        bool SetSockNonBlock();
        
        uint64_t GetSndTimeOut();
        uint64_t GetRecvTimeOut();
        bool SetSndTimeOut(uint64_t timeout);
        bool SetRecvTimeOut(uint64_t timeout);
        void setPipe();
        bool isTimeOut();
        steady_time_point getCloseTime();
        //连接功能
        bool Bind();
        bool Listen(int maxWaitQueue=SOMAXCONN);
        Socket::ptr Accetp();
        bool Connect(const Address::ptr peerAddress);
        void Close();

        SOCKET_STATUS getStatus(); 
        int getsock();
        
        //传输功能
        bool Send(std::string& message);
        bool Receive(std::string& message);
        ssize_t SendFromBuff(Buffer& buffer);
        ssize_t ReciveToBuff(Buffer& buffer);
        ssize_t SendFd(const char* str, size_t len);
        ssize_t ReceiveFd(char* str, size_t len);

    private:
        SocketType m_type;
        Family m_family;
        int m_socket;
        Address::ptr m_local_address;
        Address::ptr m_peer_address;
        SOCKET_STATUS m_status;
        steady_time_point close_time;
    };
    //根据Address创建Socket
    Socket::ptr CreateSocket(SocketType sockType, Address::ptr localAddress);
    //创建TCP-Socket
    Socket::ptr CreateTCPSocket(Address::ptr localAddress=std::make_shared<Address>());
    Socket::ptr CreateTCPSocket(Family netType, char* ip, uint16_t port);
    Socket::ptr CreateTCPSocket(Family netType, std::string& ip, uint16_t port);
    //创建UDP-Socket
    Socket::ptr CreateUDPSocket(Address::ptr localAddress);
    Socket::ptr CreateUDPSocket(Family netType, char* ip, uint16_t port);
    Socket::ptr CreateUDPSocket(Family netType, std::string& ip, uint16_t port);
    //根据管道Fd创建Socket：统一时间源
    Socket::ptr CreatePipeSocket(int fd);
    
}


#endif