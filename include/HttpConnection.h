#pragma once
#ifndef _HTTPCONNECTION_H_
#define _HTTPCONNECTION_H_

#include <Buffer.h>
#include <Socket.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <sys/stat.h>
#include <unistd.h>

using std::string;
using std::unordered_map;
using std::unordered_set;

namespace Heeb {

    class HttpReq {
    public:
        enum PARSE_STATE {
            REQUEST_LINE,
            HEADERS,
            BODY,
            FINISH
        };
        HttpReq();
        void init();
        int parseRequest(Buffer& buf);
        bool isKeepAlive() const;
        const string& getUrl() const;

    private:
        bool parseRequestLine(const string& requestLine);
        bool parseHeadLine(const string& headLine);
        bool parseBody(const string& body);


        string m_method;
        string m_url;
        string m_version;
        unordered_map<string, string> m_header;
        unordered_map<string, string> m_post;
        PARSE_STATE m_state;

        static unordered_map<string, bool(*)(const string&, HttpReq*)> m_bodyParseFun;
        static bool parseUrlencoded(const string& body, HttpReq* thisReq);

    };

    class HttpRes {
    public:
        HttpRes(const string& resrcPath);
        ~HttpRes();
        void init(const HttpReq& m_request);   //初始化
        void makeResponse(Buffer& buffer);      //生成http响应报文
        void makeErrorResponse(Buffer& buffer); //生成错误报文
        char* getContent();                     //获取请求文件的内存映射
        size_t getContentLength();              //获取请求文件长度

    private:
        void calUrlStatus();                //根据url计算状态码
        bool isFileAvailable();             //判断文件是否存在
        void makeStateLine(Buffer& buffer); //生成状态行
        void makeHeadLine(Buffer& buffer);  //生成头部行
        bool addContentMap(const string& path);//添加请求内容文件映射
        void unmmapContent();               //解除请求文件的内存映射

        string m_urlPath;
        string m_resrcPath;
        bool is_keepAlive;
        int m_StatusCode;
        char* m_content;
        struct stat m_contentStat;

        static const unordered_map<string, string> FILE_TYPE;
        static const unordered_map<int, const string> STATUS_CODE;
        static const unordered_set<string> URL_PATH;
        //static const unordered_map<int, string> ERROR_HTML;

    };

    class HttpCon {
    public:
        enum PROCESS_STATE {
            RECV_REQUEST,
            PARSE_REQUEST,
            ERROR_REQUEST,
            MAKE_RESPONSE
        };
        typedef std::shared_ptr<HttpCon> ptr;
        HttpCon(const string& resrcPath, Socket::ptr m_sock);
        void init();
        PROCESS_STATE getStatus();
        ssize_t recv();
        ssize_t send();
        size_t readBufferSize();
        size_t writeBufferSize();
        void process();
        char* getFile();
        size_t getFileSize();

    private:
        Socket::ptr m_socket;
        Buffer m_ReadBuffer;
        Buffer m_WriteBuffer;
        HttpReq m_httpReq;
        HttpRes m_httpRes;
        PROCESS_STATE m_state;
    };


}

#endif