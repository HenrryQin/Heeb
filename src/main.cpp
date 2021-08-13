#include "../include/WebServer.h"
#include "../include/Log.h"

using namespace Heeb;

int webServThread(int argc, char *argv[]) {
    int threadNum=20;
    uint16_t port=8081;
    string srcPath="resources";
    string logPath="log/";
    int queueSize=20;
    int opt;
    const char *str="n::p::s::l::q::";
    while(opt=getopt(argc, argv, str) !=-1) {
        switch (opt)
        {
        case 'n':
            threadNum=atoi(optarg);
            break;
        case 'p':
            port=atoi(optarg);
            break;
        case 's':
            srcPath=string(optarg);
            break;
        case 'l':
            logPath=string(optarg);
            break;
        case 'q':
            queueSize=atoi(optarg);
            break;           
        default:
            break;
        }
    }
    webserver::GetInstance()->init(threadNum, srcPath, port, queueSize, logPath);
    webserver::GetInstance()->run();
    //WebServer::GetInstance()->init(threadNum, srcPath, port, queueSize, logPath);
    //WebServer::GetInstance()->run();
    return 0;
}

int main(int argc, char *argv[])
{
    Thread WebServerThread("WebServerThread", webServThread, argc, argv);
    WebServerThread.join();
    return 0;
    
}
