#ifndef NETCON_H
#define NETCON_H

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <mutex>
#include <memory>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <map>
#include <set>
#include <fstream>

#include "common.h"
#include "StringBuf.h"
#include "MapBuf.h"
#include "SetBuf.h"
#include "threadpool.h"

#define MAXCON 1024
using ReadTask = std::function<void(void*)>;

class SockServer : public std::enable_shared_from_this<SockServer>{
private:
    SockServer();
    ~SockServer();

private:
    static std::shared_ptr<SockServer> mInstance;
    static void Create(void);
    static void DeleteFunc(SockServer* sock);

public:
    static std::shared_ptr<SockServer> GetInstance(void);
    void Work(void);
    void ListenWork(void);
 
private:
    void EpollOper(int fd, EpOper RorW, EpOper AorD, void* data);   //EPOLL操作
    void SetNoBlock(int fd);    //设置非阻塞
    int  ReadData(void* data);
    void CloseCon(void* data);          //断开连接
    void HeadAnalysis(void* data);      //数据头解析
    void ConnectFunc(void* data);       //建立连接
    void ReadFunc(void* data);          //读取数据
    void WriteFunc(void* data, iovec* iobuf, int iocount);         //写入数据
    void RequestProcess(void* data);    //请求处理
    void BGSave(void);
private:
    int listenFd;
    int epFd;
    std::fstream saveFIle;
    epoll_event evBuf[MAXCON];
    BaseBuf* baseBuf;
    ConBuf* ListenConBuf;
    std::map<char, BaseBuf*> bufType;
    std::shared_ptr<LockFreeQueue<ReadTask>> ReadTaskQueue;  //请求处理事件无锁队列  主线程负责读取和处理
    CThread* threadPool;
    std::mutex sockMutex;           //用于唤醒主线程进行命令处理
    std::condition_variable weakUp;
};


#endif