#include "../include/Socket.h"
#include <arpa/inet.h>
#include <exception>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include "../include/Log.h"

namespace Heeb {
    /*************************地址封装*********************************/
    //创建默认地址
    Address::Address(Family netType, uint16_t port):m_family(netType) {
        if(m_family == IPv4) {
            bzero(&m_address, sizeof(m_address));
            m_address.address4.sin_family = IPv4;
            m_address.address4.sin_addr.s_addr = htonl(INADDR_ANY);
            m_address.address4.sin_port = htons(port);
        }
        else {
            m_address.address6.sin6_family = IPv6;
            m_address.address6.sin6_addr = in6addr_any;
            m_address.address6.sin6_port = htons(port);
        }
    }

    //创建ipv4地址
    Address::Address(uint32_t ip_4, uint16_t port):m_family(IPv4) {
        m_address.address4.sin_family = IPv4;
        m_address.address4.sin_addr.s_addr = htonl(ip_4);
        m_address.address4.sin_port = htons(port);
    }

    //创建ipv6地址(地址必须是网络字节序), 端口为主机字节序
    Address::Address(in6_addr addr, uint16_t port):m_family(IPv6) {
        m_address.address6.sin6_family = IPv6;
        m_address.address6.sin6_addr = addr;
        m_address.address6.sin6_port = htons(port);
    }

    //根据字符串创建地址
    Address::Address(Family netType, const char* ip, uint16_t port):m_family(netType) {
        if(m_family==IPv4) {
            m_address.address4.sin_family = IPv4;
            try{
                if(inet_pton(IPv4, ip, &(m_address.address4.sin_addr.s_addr))!=1) {
                    throw(std::runtime_error("invalid ip! change it to addr_any!"));
                }
            }
            catch (std::runtime_error) {
                m_address.address4.sin_addr.s_addr = htonl(INADDR_ANY);
            }
            
            m_address.address4.sin_port = htons(port);
        }
        else {
            m_address.address6.sin6_family = IPv6;
            try{
                if(inet_pton(IPv6, ip, &(m_address.address6.sin6_addr)) != 1) {
                    throw(std::runtime_error("invalid ip! change it to addr_any6!"));
                }
            }
            catch (std::runtime_error) {
                m_address.address6.sin6_addr = in6addr_any;
            }
            m_address.address6.sin6_port = htonl(port);
        }
    }

    Address::Address(Family netType, const std::string& ip, uint16_t port) {
        Address(netType, ip.c_str(), port);
    }

    //根据sockaddr_in(6)构造
    Address::Address(const sockaddr_in& sockaddr_4):m_family(IPv4) {
        m_address.address4 = sockaddr_4;
    }
    Address::Address(const sockaddr_in6& sockaddr_6):m_family(IPv6) {
        m_address.address6 = sockaddr_6;
    }

    const Family& Address::GetFamily() {return m_family;}

    sockaddr* Address::GetSockAddr() {
        if(m_family=IPv4) return (sockaddr*) (&(m_address.address4));
        return (sockaddr*) (&(m_address.address6));
    }

    socklen_t Address::GetSocklen() {
        if(m_family=IPv4) return sizeof(sockaddr_in);
        return sizeof(sockaddr_in6);
    }

    sockaddr_in Address::GetSockAddrIn() {
        return m_address.address4;
    }
    /*************************地址封装*********************************/

    /************************socket封装*********************************/
    const std::chrono::milliseconds WAITTIME(1000*10);
     //构造
    Socket::Socket(SocketType stype, Address::ptr localAddress):m_type(stype), 
                                                                m_family(localAddress->GetFamily()), 
                                                                m_local_address(localAddress),
                                                                m_status(INVALID){
        m_socket=socket(m_family, m_type, 0);
        if(m_socket<0) {
            std::cout<<"Socket() Error:m_socket<0"<<std::endl;
            return;
        }
        m_status=INIT;
        
    }

    Socket::Socket(SocketType stype, Address::ptr localAddress, int socketfd): m_type(stype), 
                                                                            m_family(localAddress->GetFamily()), 
                                                                            m_local_address(localAddress), 
                                                                            m_socket(socketfd),
                                                                            m_status(INVALID) {
        if(m_socket<0) {
            std::cout<<"Socket() Error:m_socket<0"<<std::endl;
            return;
        }
        m_status=INIT;
    }

    //参数读取、设置
    bool Socket::GetSockOpt(int level, int optName, void* optVal, socklen_t* optLen) {
        if(m_status==INVALID) {
            std::cout<<"GetSockOpt() Error: Invalid Socket"<<std::endl;
            return false;
        }
        if(0 != getsockopt(m_socket, level, optName, optVal, optLen)) {
            std::cout<<"GetSockOpt() Error"<<std::endl;
            return false;
        }
        return true;
    }

    bool Socket::SetSockOpt(int level, int optName, void* optVal, socklen_t optLen) {
        if(m_status==INVALID) {
            std::cout<<"SetSockOpt() Error: Invalid Socket"<<std::endl;
            return false;
        }
        if(0 != setsockopt(m_socket, level, optName, optVal, optLen)) {
            std::cout<<"SetSockOpt() Error"<<std::endl;
            return false;
        }
        return true;
    }

    uint64_t Socket::GetSndTimeOut() {
        if(m_status==INVALID) {
            std::cout<<"GetSndTimeOut() Error: Invalid Socket"<<std::endl;
            return 0;
        }
        timeval timeout;
        if(GetSockOpt(SOL_SOCKET, SO_SNDTIMEO, timeout)) {
            return timeout.tv_sec * 1000 + timeout.tv_usec;
        }
        std::cout<<"GetSndTimeOut() Error"<<std::endl;
        return 0;
    }
    uint64_t Socket::GetRecvTimeOut() {
        if(m_status==INVALID) {
            std::cout<<"GetRecvTimeOut() Error: Invalid Socket"<<std::endl;
            return false;
        }
        timeval timeout;
        if(GetSockOpt(SOL_SOCKET, SO_RCVTIMEO, timeout)) {
            return timeout.tv_sec * 1000 + timeout.tv_usec;
        }
        std::cout<<"GetRecvTimeOut() Error"<<std::endl;
        return 0;
    }

    bool Socket::SetSndTimeOut(uint64_t timeout) {
        if(m_status==INVALID) {
            std::cout<<"SetSndTimeOut() Error: Invalid Socket"<<std::endl;
            return false;
        }
        timeval time;
        time.tv_sec = timeout/1000;
        time.tv_usec = timeout%1000;
        if(SetSockOpt(SOL_SOCKET, SO_SNDTIMEO, time)) return true;
        std::cout<<"SetSndTimeOut() Error"<<std::endl;
        return false;
    }

    bool Socket::SetRecvTimeOut(uint64_t timeout) {
        if(m_status==INVALID) {
            std::cout<<"SetRecvTimeOut() Error: Invalid Socket"<<std::endl;
            return false;
        }
        timeval time;
        time.tv_sec = timeout/1000;
        time.tv_usec = timeout%1000;
        if(SetSockOpt(SOL_SOCKET, SO_RCVTIMEO, time)) return true;
        std::cout<<"SetRecvTimeOut() Error"<<std::endl;
        return false;
    }

    bool Socket::SetSockNonBlock() {
        int opt=fcntl(m_socket, F_GETFL);
        opt=opt|O_NONBLOCK;
        return fcntl(m_socket, F_SETFL, opt) == 0;
    }

    steady_time_point Socket::getCloseTime() {
        return close_time;
    }

    bool Socket::isTimeOut() {
        return close_time <= std::chrono::steady_clock::now();
    }

    //连接功能
    bool Socket::Bind() {
        if(m_status!=INIT) {
            std::cout<<"Bind() Error: Invalid Socket"<<std::endl;
            return false;
        }
        //sockaddr_in addr=m_local_address->GetSockAddrIn();
        //socklen_t addrlen=m_local_address->GetSocklen();
        //int res=bind(m_socket, (struct sockaddr*)&addr, addrlen);
        if(bind(m_socket, m_local_address->GetSockAddr(), m_local_address->GetSocklen()) < 0) {
            LogManager::GetInstance()->log("WebServerLog", LogLevel::ERROR, "Socket.cpp", 219, "BindFailed:"+std::to_string(errno), LogLevel::ERROR);
            return false;
        }
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "Socket.cpp", 222, "BindSuccessful", LogLevel::INFO);
        m_status=BIND;
        return true;
    }

    bool Socket::Listen(int maxWaitQueue) {
        if(m_status!=BIND) {
            std::cout<<"Listen() Error: Invalid Socket"<<std::endl;
            return false;
        }
        if(listen(m_socket, maxWaitQueue)<0) {
            LogManager::GetInstance()->log("WebServerLog", LogLevel::ERROR, "Socket.cpp", 232, "ListenFailed:"+std::to_string(errno), LogLevel::ERROR);
            return false;
        }
        m_status=LISTEN;
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "Socket.cpp", 238, "ListenSuccess", LogLevel::INFO);
        return true;
    }

    Socket::ptr Socket::Accetp() {
        if(m_status!=LISTEN) {
            std::cout<<"Accept() Error: Invalid Socket"<<std::endl;
            return std::make_shared<Socket>(*this);
        }
        socklen_t socklen = m_local_address->GetSocklen();
        int accsocket = accept(m_socket, m_local_address->GetSockAddr(), &socklen);
        if(accsocket < 0) {
            std::cout<<"Accept() Error"<<std::endl;
            return std::make_shared<Socket>(*this);
        }
        Socket::ptr conSocket = std::make_shared<Socket>(this->m_type, this->m_local_address, accsocket);
        conSocket->m_status=CONNECT;
        conSocket->close_time=std::chrono::steady_clock::now()+WAITTIME;
        return conSocket;
    }

    bool Socket::Connect(const Address::ptr peerAddress) {
        if(m_status!=BIND) {
            std::cout<<"Connect() Error: Invalid Socket"<<std::endl;
            return false;
        }
        if(connect(m_socket, peerAddress->GetSockAddr(), peerAddress->GetSocklen())<0) {
            std::cout<<"Connect() Error"<<std::endl;
            return false;
        }
        m_peer_address=peerAddress;
        m_status=CONNECT;
        close_time=std::chrono::steady_clock::now()+WAITTIME;
        return true;
    }

    void Socket::Close() {
        if(close(m_socket) < 0) {
            LogManager::GetInstance()->log("WebServerLog", LogLevel::ERROR, "Socket.cpp", 275, "CloseFailed:"+std::to_string(errno), LogLevel::ERROR);
            m_status=INVALID;
            return;
        }
        m_status=CLOSE;
    }
    
    Socket::SOCKET_STATUS Socket::getStatus() {return m_status;}

    int Socket::getsock() {return m_socket;}
    
    void Socket::setPipe() {m_status=PIPE;}


    //传输功能
    bool Socket::Send(std::string& message) {
        if(m_type==TCP) {
            if(m_status!=CONNECT) {
                std::cout<<"Send() Error: Invalid Socket"<<std::endl;
                return false;
            }
            int errorCount=0;
            int send_size=0;
            while(send_size<message.size()) {
                int size = send(m_socket, message.c_str()+send_size, message.size()-send_size, 0);
                if(size<0) {
                    ++errorCount;
                    std::cout<<"send error!"<<std::endl;
                    if(errorCount==10) return false;
                }
            }
            close_time=std::chrono::steady_clock::now()+WAITTIME;
        }
        else {
            if(m_status!=BIND) {
                std::cout<<"Send() Error: Invalid Socket"<<std::endl;
                return false;
            }
            int sent_size=sendto(m_socket, message.c_str(), message.size(), 0, m_peer_address->GetSockAddr(), m_peer_address->GetSocklen());
            if(sent_size<0) {
                std::cout<<"send error!"<<std::endl;
                return false;
            }
            else if(sent_size<message.size()) {
                std::cout<<"sent incompletly"<<std::endl;
                return false;
            }
        }        
        return true;

        
    }

    bool Socket::Receive(std::string& message) {
        char buf[1024];
        if(m_type==TCP) {
            if(m_status!=CONNECT) {
                std::cout<<"Reveive() Error: Invalid Socket"<<std::endl;
                return false;
            }
            if(recv(m_socket, buf, 1024, 0)<0) {
                std::cout<<"Receive() Error"<<std::endl;
                return false;
            }
            close_time=std::chrono::steady_clock::now()+WAITTIME;
        }
        else {
            if(m_status!=BIND) {
                std::cout<<"Reveive() Error: Invalid Socket"<<std::endl;
                return false;
            }
            socklen_t len = m_peer_address->GetSocklen();
            if(recvfrom(m_socket, buf, 1024, 0, m_peer_address->GetSockAddr(), &len)<0) {
                std::cout<<"Receive() Error"<<std::endl;
                return false;
            }
        }
        message = buf;
        return true;
        
    }
    
    ssize_t Socket::SendFromBuff(Buffer& buffer) {
        ssize_t len = buffer.writeFd(m_socket);
        close_time=std::chrono::steady_clock::now()+WAITTIME;
        return len;
    }

    ssize_t Socket::ReciveToBuff(Buffer& buffer) {
        ssize_t len = buffer.readFd(m_socket);
        close_time=std::chrono::steady_clock::now()+WAITTIME;
        return len;
    }

    ssize_t Socket::SendFd(const char* str, size_t len) {
        return write(m_socket, (const void*) str, len);
    }

    ssize_t Socket::ReceiveFd(char* str, size_t len) {
        return read(m_socket, (void*) str, len);
    }

    //根据Address创建Socket
    Socket::ptr CreateSocket(SocketType sockType, Address::ptr localAddress) {
        return std::make_shared<Socket>(sockType, localAddress);
    }
    //创建TCP-Socket
    Socket::ptr CreateTCPSocket(Address::ptr localAddress) {
        return std::make_shared<Socket>(TCP, localAddress);
    }
    Socket::ptr CreateTCPSocket(Family netType, char* ip, uint16_t port) {
        Address::ptr address = std::make_shared<Address>(netType, ip, port);
        Socket::ptr sock = std::make_shared<Socket>(TCP, address);
        return sock;
    }
    Socket::ptr CreateTCPSocket(Family netType, std::string& ip, uint16_t port) {
        Address::ptr address = std::make_shared<Address>(netType, ip, port);
        Socket::ptr sock = std::make_shared<Socket>(TCP, address);
        return sock;
    }
    //创建UDP-Socket
    Socket::ptr CreateUDPSocket(Address::ptr localAddress) {
        Socket::ptr s = std::make_shared<Socket>(UDP, localAddress);
        s->Bind();
    }
    Socket::ptr CreateUDPSocket(Family netType, char* ip, uint16_t port) {
        Address::ptr address = std::make_shared<Address>(netType, ip, port);
        Socket::ptr sock = std::make_shared<Socket>(UDP, address);
        return sock;
    }
    Socket::ptr CreateUDPSocket(Family netType, std::string& ip, uint16_t port) {
        Address::ptr address = std::make_shared<Address>(netType, ip, port);
        Socket::ptr sock = std::make_shared<Socket>(UDP, address);
        return sock;
    }
    //根据管道Fd创建Socket：统一时间源
    Socket::ptr CreatePipeSocket(int fd) {
        std::shared_ptr<Socket> pipePtr = std::make_shared<Socket>(TCP, std::make_shared<Address>(PIPE, 0), fd);
        return pipePtr;        
    }

    
    /************************socket封装*********************************/

}