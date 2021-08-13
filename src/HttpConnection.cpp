#include <../include/HttpConnection.h>
#include <regex>
#include <unistd.h>
#include <mysql/mysql.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include "../include/Log.h"

#define MAX_PARSE_COUNT 100;
namespace Heeb {
    bool HttpReq::parseUrlencoded(const string& body, HttpReq* thisReq) {
        string key,value;
        bool keyOrValue=true;  
        for(int i=0; i<body.size(); ++i) {
            switch (body[i])
            {
            case '+':
                {
                    if(keyOrValue) key+=' ';
                    else value+=' ';
                    break;
                }
                
            case '%':
                {
                    if(i+3>=body.size()) {
                        cout<<"body error"<<endl;
                        return false;
                    }
                    string num=body.substr(i+1, 2);
                    char c=std::stoi(num, nullptr, 16);
                    if(keyOrValue) key+=c;
                    else value+=c;
                    i+=2;
                    break;
                }
            case '=':
                {
                    keyOrValue=false;
                    break;
                }
            case '&':
                {
                    thisReq->m_post[key]=value;
                    keyOrValue=true;
                }    
            default:
                if(keyOrValue) key+=body[i];
                else value+=body[i];
                break;
            }
        }
        if(thisReq->m_post.find(key)==thisReq->m_post.end()) thisReq->m_post[key]=value;
        return true;
    }

    unordered_map<string, bool(*)(const string&, HttpReq*)> HttpReq::m_bodyParseFun{
        {"application/x-www-from-urlencoded", &HttpReq::parseUrlencoded}
    };

    HttpReq::HttpReq():m_state(REQUEST_LINE) { }

    void HttpReq::init() {
        m_method=m_url=m_version="";
        m_header.clear();
        m_post.clear();
        m_state=REQUEST_LINE;
    }
    
    //解析http请求
    int HttpReq::parseRequest(Buffer& buf) {
        string line;
        while (m_state<BODY) {
            if(!buf.readHttpLine(line)) {
                cout<<"readHttpLine failed!"<<endl;
                return -1;//读取http行失败返回-1；
            }
            switch (m_state) {
            case REQUEST_LINE: 
                if(parseRequestLine(line)) m_state=HEADERS;
                else {
                    cout<<"parseRequestLine failed!"<<endl;
                    return 1;//解析请求行失败返回1
                }
                break;
            case HEADERS:
                if(line=="\r\n") { 
                    if(m_header.find("Content-Length")!=m_header.end()) m_state=BODY;
                    else m_state=FINISH; 
                }
                else if(!parseHeadLine(line)) {
                    cout<<"parseHeadLine failed!"<<endl;
                    return 2;//解析头部行失败返回2
                } 
                break;
            default:
                break;
            }
        }
        if(m_state==BODY) {
            size_t length = std::stoi(m_header["Content-Length"]);
            if(!buf.readHttpBody(line, length)) {
                cout<<"readHttpBody failed"<<endl;
                return -2;//读取body失败返回-2
            }
            if(!parseBody(line)) {
                cout<<"parseBody failed"<<endl;
                return 3;//解析body失败返回3
            }
        }
        return 0;        
    }

    bool HttpReq::isKeepAlive() const {
        auto iter=m_header.find("Connection");
        return iter->second=="keep-alive" && m_version=="1.1";
    }

    const string& HttpReq::getUrl()const {return m_url;}

    bool HttpReq::parseRequestLine(const string& requestLine) {
        std::regex RequestLinePattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)\r\n");
        std::smatch result;
        if(!std::regex_match(requestLine, result, RequestLinePattern)) return false;
        m_method=result[1];
        m_url=result[2];
        m_version=result[3];
        return true;
    }

    bool HttpReq::parseHeadLine(const string& headLine) {
        std::regex RequestLinePattern("^([^:]*): (.*)\r\n");
        std::smatch result;
        if(!std::regex_match(headLine, result, RequestLinePattern)) return false;
        m_header[result[1]]=result[2];
        return true;
    }
    bool HttpReq::parseBody(const string& body) {
        auto iter=m_header.find("Content-Type");
        if(iter==m_header.end()) {
            cout<<"cann't find Content-Type in headers"<<endl;
            return false;
        }
        return m_bodyParseFun[iter->second](body, this);
    }

    const unordered_map<string, string> HttpRes::FILE_TYPE{
        {".htm", "text/html"},
        {".html", "text/html"},
        {".htx", "text/html"},
        {".xhtml", "application/xhtml+xml"},
        {".rtf", "application/rtf"},
        {".word", "application/nsword"},
        {".txt", "text/plain"},
        {".xml", "text/xml"},
        {".au", "audio/basic"},
        {".bmp", "application/x-bmp"},
        {".gif", "image/gif"},
        {".img", "image/img"},
        {".jpe", "image/jpeg"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".tif", "image/tiff"},
        {".tiff", "image/tiff"},
        {".png", "image/png"},
        {".mp1", "audio/mp1"},
        {".mp2", "audio/mp2"},
        {".mp3", "audio/mp3"},
        {".wma", "application/x-ms-wma"},
        {".mp4", "video/mpeg4"},
        {".mpeg", "video/mpeg"},
        {".avi", "video/x-msvideo"},
        {".rmvb", "application/vnd.rn-realmedia-vbr"},
        {".pdf", "application/pdf"},
        {".css", "text/css"},
        {".js", "text/javascript"},
        {".gz", "application/x-gzip"},
        {".tar", "application/x-tar"}
        
    };

    const unordered_map<int, const string> HttpRes::STATUS_CODE {
        {200, "OK"},
        {204, "No Content"},
        {301, "Moved Permanently"},
        {400, "Bad Request"},
        {403, "Forcidden"},
        {404, "Not Found"},
        {500, "Internal Server Error"},
        {503, "Service Unavailable"}
    };

    const unordered_set<string> HttpRes::URL_PATH {
        "/welcome", "/home", "/login", "/picture", "/video", "/music"
    };
    /*
    unordered_map<string, string> HttpRes::ERROR_HTML {
        {400, "/400.html"},
        {403, "/403.html"},
        {404, "/404.html"}
    };
    */
    HttpRes::HttpRes(const string& resrcPath):m_resrcPath(resrcPath) {  }
    HttpRes::~HttpRes() {
        unmmapContent();
    }

    //初始化
    void HttpRes::init(const HttpReq& m_request) {
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "HttpCon.cpp", 202, "enter http_res init", LogLevel::INFO);
        unmmapContent();
        m_urlPath=m_request.getUrl();
        is_keepAlive=m_request.isKeepAlive();
        calUrlStatus();
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "HttpCon.cpp", 207, "exit http_res init", LogLevel::INFO);

    }   

    //生成http响应报文
    void HttpRes::makeResponse(Buffer& buffer) {
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "HttpCon.cpp", 213, "enter makeResponse()", LogLevel::INFO);
        makeStateLine(buffer);
        makeHeadLine(buffer);
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "HttpCon.cpp", 216, "exit makeResponse()", LogLevel::INFO);
    }      
    
    //生成错误报文
    void HttpRes::makeErrorResponse(Buffer& buffer) {
        string reponse="HTTP/1.1 400 Bad Request\r\nConnection: ";
        if(is_keepAlive) reponse+="keep-alive: max=6, timeout=120\r\n";
        else reponse+="Close\r\n";
        buffer.writeBuff(reponse);
    }

    //获取请求文件的内存映射
    char* HttpRes::getContent() {
        return m_content;
    }

    //获取请求文件长度
    size_t HttpRes::getContentLength() {
        return m_contentStat.st_size;
    } 

    //判断文件是否存在，若存在则映射文件
    bool HttpRes::isFileAvailable() {
        string path=m_resrcPath+m_urlPath;
        if(access(path.c_str(), R_OK)<0) {
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "HttpCon.cpp", 239, "access fail: "+std::to_string(errno)+path, LogLevel::INFO);
            return false;
        }
        return addContentMap(path);
    } 

    //根据url计算状态码
    void HttpRes::calUrlStatus() {

        if(m_urlPath=="/") m_urlPath="/welcome.html";
        else if(URL_PATH.find(m_urlPath)!=URL_PATH.end()) m_urlPath+=".html";

        if(isFileAvailable()) {
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "HttpCon.cpp", 254, "file mapped", LogLevel::INFO);
            m_StatusCode=200;
        }
        else {
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "HttpCon.cpp", 258, "file to 404", LogLevel::INFO);
            m_urlPath="/404.html";
            m_StatusCode=404;
            if(!addContentMap(m_resrcPath+m_urlPath)) {
                LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "HttpCon.cpp", 262, "404 map fail", LogLevel::INFO);
                m_contentStat={0};
                m_content=nullptr;
            } 
            else LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "HttpCon.cpp", 266, "404 mmaped", LogLevel::INFO);           
        }
    } 

    //生成状态行
    void HttpRes::makeStateLine(Buffer& buffer) {
        string status;
        auto iter=STATUS_CODE.find(m_StatusCode);
        if(iter!=STATUS_CODE.end()) status= iter->second;
        else {
            iter=STATUS_CODE.find(400);
            status=iter->second;
        }
        string stateLine="HTTP/1.1 "+std::to_string(m_StatusCode)+" "+status+"\r\n";
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "HttpCon.cpp", 280, stateLine, LogLevel::INFO);
        buffer.writeBuff(stateLine);
    }

    //生成头部行
    void HttpRes::makeHeadLine(Buffer& buffer) {
        
        string HeadLine="Connection: ";
        if(is_keepAlive) HeadLine+="keep-alive\r\nkeep-alive: max=6, timeout=120\r\n";
        else HeadLine+="close\r\n";

        string type="text/plain";
        size_t dotPos=m_urlPath.find_last_of('.');
        auto iter=FILE_TYPE.find(m_urlPath.substr(dotPos));
        if(iter!=FILE_TYPE.end()) type=iter->second;
        HeadLine+="Content-type: "+type+"\r\n";
        HeadLine+="Content-length: "+std::to_string(getContentLength())+"\r\n\r\n";
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "HttpCon.cpp", 297, HeadLine, LogLevel::INFO);
        buffer.writeBuff(HeadLine);     
    }

    //添加请求内容文件映射
    bool HttpRes::addContentMap(const string& path) {
        
        int contentFd=open(path.c_str(), O_RDONLY);
        if(contentFd<0) {
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "HttpCon.cpp", 299, "openfail fail: "+std::to_string(errno)+path, LogLevel::INFO);
            return false;
        }
        if(stat(path.c_str(), &m_contentStat)<0) return false;
        int* addr=(int*) mmap(NULL, m_contentStat.st_size, PROT_READ, MAP_PRIVATE, contentFd, 0);
        close(contentFd);
        if(*addr==-1) {
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "HttpCon.cpp", 308, "mmap fail: "+std::to_string(errno), LogLevel::INFO);
            return false;
        }
        m_content=(char*) addr;
        return true;
    }               

    //解除请求文件的内存映射
    void HttpRes::unmmapContent() {
        if(m_content) {
            munmap(m_content, m_contentStat.st_size);
            m_content=nullptr;
            m_contentStat={0};
        }
    }         

    HttpCon::HttpCon(const string& resrcPath, Socket::ptr m_sock):m_httpRes(resrcPath), m_socket(m_sock) {
        m_httpReq.init();
    }

    void HttpCon::init() {
        m_httpReq.init();
        m_state=RECV_REQUEST;
    }
    HttpCon::PROCESS_STATE HttpCon::getStatus() {return m_state;}

    ssize_t HttpCon::recv() {
        return m_socket->ReciveToBuff(m_ReadBuffer);
    }
    ssize_t HttpCon::send() {
        return m_socket->SendFromBuff(m_WriteBuffer);
    }

    size_t HttpCon::readBufferSize() {
        return m_ReadBuffer.readableSize();
    }
    size_t HttpCon::writeBufferSize() {
        return m_WriteBuffer.readableSize();
    }
    
    void HttpCon::process() {
        m_state=PARSE_REQUEST;
        LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "HttpCon.cpp", 334, "enter process", LogLevel::INFO);
        int parseRes = m_httpReq.parseRequest(m_ReadBuffer);
        if(parseRes<0) {
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "HttpCon.cpp", 338, "http incomplete", LogLevel::INFO);
            m_state=RECV_REQUEST;
            return;
        }
        else if(parseRes==0) {
            LogManager::GetInstance()->log("WebServerLog", LogLevel::INFO, "HttpCon.cpp", 342, "http parseSuccess", LogLevel::INFO);
            m_httpRes.init(m_httpReq);
            m_state=MAKE_RESPONSE;
            m_httpRes.makeResponse(m_WriteBuffer);
        }
        else {
            m_httpRes.init(m_httpReq);
            m_state=ERROR_REQUEST;
            m_httpRes.makeErrorResponse(m_WriteBuffer);
        }
    }

    char* HttpCon::getFile() {
        return m_httpRes.getContent();
    }
    size_t HttpCon::getFileSize() {
        return m_httpRes.getContentLength();
    }

}