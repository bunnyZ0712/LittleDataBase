#include <stdexcept>
#include <cstring>
#include <signal.h>
#include "NetCon.h"

std::shared_ptr<SockServer> SockServer::mInstance = nullptr;

inline int GetColon(const char* buf)
{
    int n = strlen(buf);

    for(int i = 0; i < n; i++)
    {
        if(buf[i] == ':')
            return i;
    }

    return -1;
}

SockServer::SockServer()
{
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    listenFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(listenFd < 0)
    {
        std::cout << "socket failed" << std::endl;
    }
    
    int reuseaddr = 1;
    if(setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, (const void*)&reuseaddr, sizeof(reuseaddr)) < 0)
    {
        std::cout << "setsockopt failed" << std::endl;
    }
    if(bind(listenFd, (sockaddr*)&serverAddr, sizeof(sockaddr)) < 0)
    {
        std::cout << "bind failed" << std::endl;
    }
    if(listen(listenFd, MAXCON) < 0)
    {
        std::cout << "listen failed" << std::endl;
    }

    epFd = epoll_create(MAXCON);
    if(epFd < 0)
    {
        std::cout << "epoll_create failed" << std::endl;
    }
    ListenConBuf = new ConBuf;
    ListenConBuf ->fd = listenFd;
    ListenConBuf ->ReadFunc = std::bind(&SockServer::ConnectFunc, this, ListenConBuf);
    EpollOper(listenFd, READ, ADD, ListenConBuf);
    bufType[OperBufType::STRING] = new StringBuf(10);
    bufType[OperBufType::MAP] = new MapBuf(10);
    bufType[OperBufType::SETS] = new SetBuf(10);
    ReadTaskQueue = LockFreeQueue<ReadTask>::GetInstance();

    threadPool = CThread::GetInstance(3, 3);
}

void SockServer::DeleteFunc(SockServer* sock)
{
    for(auto& it : sock ->bufType)
    {
        delete it.second;
    }
    delete sock ->ListenConBuf;
    sock ->ListenConBuf = nullptr;
    if(sock != nullptr)
    {
        delete sock;
        sock = nullptr;
    }
}

SockServer::~SockServer()
{

}

void SockServer::Create(void)
{
    if(mInstance == nullptr)
    {
        mInstance.reset((new SockServer), DeleteFunc);
    }
}

std::shared_ptr<SockServer> SockServer::GetInstance(void)
{
    static std::once_flag CreateFlag;

    std::call_once(CreateFlag, Create);

    return mInstance ->shared_from_this();
}

void SockServer::EpollOper(int fd, EpOper RorW, EpOper AorD, void* data)
{
    epoll_event ev;

    ev.events = EPOLLET;
    ev.data.ptr = data;

    if(RorW == EpOper::READ)
    {
        ev.events |= EPOLLIN;
    }
    else if(RorW == EpOper::WRITE)
    {
        ev.events |= EPOLLOUT;
    }

    if(AorD == EpOper::ADD)
    {
        int ret = epoll_ctl(epFd, EPOLL_CTL_ADD, fd, &ev);
    }
    else if(AorD == EpOper::DELETE)
    {
        epoll_ctl(epFd, EPOLL_CTL_DEL, fd, nullptr);
    }
    else if(AorD == EpOper::MOD)
    {
        epoll_ctl(epFd, EPOLL_CTL_MOD, fd, &ev);
    }
}

void SockServer::SetNoBlock(int fd)
{
    int newstatus, oldstatus;

    oldstatus = fcntl(fd, F_GETFL);
    if(oldstatus < 0)
    {
        std::cout << "set noblock failed" << std::endl;
    }

    newstatus = oldstatus | O_NONBLOCK;

    fcntl(fd, F_SETFL, newstatus);
}

void SockServer::CloseCon(void* data)
{
    ConBuf* conbuf = static_cast<ConBuf*>(data);

    close(conbuf ->fd);
    delete[] conbuf ->readMemPointer;

    delete conbuf;
}

int SockServer::ReadData(void* data)        //读取数据
{
    ConBuf* conbuf = static_cast<ConBuf*>(data);

    int ret = recv(conbuf ->fd, conbuf ->readBuf, conbuf ->readDataLen, 0);
    
    return ret;
}

void SockServer::HeadAnalysis(void* data)    //解析头部
{
    ConBuf* conbuf = static_cast<ConBuf*>(data);

    conbuf ->readDataLen = conbuf ->headBuf.DataLen;
    conbuf ->readBuf = new char[conbuf ->readDataLen];
    conbuf ->readMemPointer = conbuf ->readBuf;
    conbuf ->readSta = READDATA;
    // std::cout << "head" << conbuf ->readDataLen << std::endl;
}

void SockServer::ReadFunc(void* data)
{
    ConBuf* conbuf = static_cast<ConBuf*>(data);
    while(true)
    {
        int ret = ReadData(conbuf);
        // std::cout << ret << std::endl;
        if(ret < 0)
            return;
        else if(ret == 0)
        {
            CloseCon(conbuf);
            return;
        }
        if(conbuf ->readSta == READHEAD)
        {
            if(ret == conbuf ->readDataLen)
            {
                HeadAnalysis(conbuf);
            }
            else
            {
                conbuf ->readDataLen -= ret;
                conbuf ->readBuf += ret;
            }
        }
        else if(conbuf ->readSta == READDATA)
        {
            if(ret == conbuf ->readDataLen)
            {
                conbuf ->readSta == READOVER;
                break;
            }      
            else
            {
                conbuf ->readDataLen -= ret;
                conbuf ->readBuf += ret;                
            }     
        } 
    }
    ReadTaskQueue ->Push(std::bind(&SockServer::RequestProcess, this, conbuf));
    weakUp.notify_one();
    // RequestProcess(conbuf);
}

void SockServer::RequestProcess(void* data)
{
    ConBuf* conbuf = static_cast<ConBuf*>(data);

    baseBuf = bufType[conbuf ->headBuf.OperBuf];
    int p = GetColon(conbuf ->readMemPointer);
    std::string ans;
    iovec iobuf[2];
    int iocount = 0;
    
    if(conbuf ->headBuf.Oper > INSERTOPER && conbuf ->headBuf.Oper < DELETEOPER)    //插入操作
    {
        auto ret = baseBuf ->Insert(conbuf ->readMemPointer, conbuf ->headBuf.Oper);
        if(ret == true)   
        {
            conbuf ->headBuf.DataLen = 0;
            conbuf ->headBuf.Oper = OperStatus::INSERTOK;
            iobuf[0].iov_base = &conbuf ->headBuf;
            iobuf[0].iov_len = HEADLEN;
            iocount = 1;
        }   
        else if(ret == false)
        {
            conbuf ->headBuf.DataLen = 0;
            conbuf ->headBuf.Oper = OperStatus::INSERTEXIST;
            iobuf[0].iov_base = &conbuf ->headBuf;
            iobuf[0].iov_len = HEADLEN;
            iocount = 1;
        }              
    }    
    else if(conbuf ->headBuf.Oper > DELETEOPER && conbuf ->headBuf.Oper < QUERYOPER)       //删除操作
    {
        auto ret = baseBuf ->Delete(conbuf ->readMemPointer, conbuf ->headBuf.Oper);
        if(ret == true)   
        {
            conbuf ->headBuf.DataLen = 0;
            conbuf ->headBuf.Oper = OperStatus::DELETOK;
            iobuf[0].iov_base = &conbuf ->headBuf;
            iobuf[0].iov_len = HEADLEN;
            iocount = 1;
        }   
        else if(ret == false)
        {
            conbuf ->headBuf.DataLen = 0;
            conbuf ->headBuf.Oper = OperStatus::DELETEERROR;
            iobuf[0].iov_base = &conbuf ->headBuf;
            iobuf[0].iov_len = HEADLEN;
            iocount = 1;
        }   
    }
    else if(conbuf ->headBuf.Oper > QUERYOPER && conbuf ->headBuf.Oper < BGSAVE)      //查询操作
    {
        auto ret = baseBuf ->Get(conbuf ->readMemPointer, &ans, conbuf ->headBuf.Oper);
        if(ret == true)   
        {
            conbuf ->headBuf.DataLen = ans.size();
            conbuf ->headBuf.Oper = OperStatus::GETOK;
            iobuf[0].iov_base = &conbuf ->headBuf;
            iobuf[0].iov_len = HEADLEN;
            iobuf[1].iov_base = const_cast<char*>(ans.c_str());
            iobuf[1].iov_len = ans.size();
            iocount = 2;
        }   
        else if(ret == false)       //没找到数据
        {
            conbuf ->headBuf.DataLen = 0;
            conbuf ->headBuf.Oper = OperStatus::NODATA;
            iobuf[0].iov_base = &conbuf ->headBuf;
            iobuf[0].iov_len = HEADLEN;
            iocount = 1;
        }                   
    }
    else if(conbuf ->headBuf.Oper == BGSAVE)
    {
        auto ret = fork();
        if(ret == 0)
        {
            BGSave();
            return;
        }
        else
        {
            return;
        }    
    }
    threadPool ->AddWriteTask(std::bind(&SockServer::WriteFunc, this, conbuf, iobuf, iocount));     //投递写事件，IO线程完成写数据
}

void SockServer::BGSave(void)
{
    saveFIle.open("save.txt", std::ios::in | std::ios::out | std::ios::trunc);
    bufType[OperBufType::STRING]->Save(saveFIle);
    
    raise(SIGKILL);
}

void SockServer::WriteFunc(void* data, iovec* iobuf, int iocount)
{
    ConBuf* conbuf = static_cast<ConBuf*>(data);
    // std::cout << "write" << std::this_thread::get_id() << std::endl;
    writev(conbuf ->fd, iobuf, iocount);
    conbuf ->readSta = READHEAD;
    conbuf ->readDataLen = HEADLEN;
    conbuf ->readBuf = (char*)&conbuf ->headBuf;
    // baseBuf->Display();
}

void SockServer::Work(void) //负责处理命令请求
{
    ReadTask readtask;
    auto condition = [&]() ->bool{
        if(ReadTaskQueue ->size() > 0)
            return true;
        else
            return false;
    };
    pid_t flag = getpid();
    while(true)
    {
        std::unique_lock<std::mutex> SockMutex(sockMutex);
        weakUp.wait(SockMutex, condition);
        while(ReadTaskQueue ->Pop(readtask) == true)    //当有任务时一直处理，没有任务则进入睡眠
        {
            readtask(nullptr);
        }
    }
}

void SockServer::ConnectFunc(void* data)
{
    sockaddr_in ClientAddr;
    socklen_t SockLen = sizeof(sockaddr);

    int Confd = accept(listenFd, (sockaddr*)&ClientAddr, &SockLen); //取出连接
    if(Confd < 0)
    {
        std::cout << "accept failed" << std::endl;
    }

    SetNoBlock(Confd);   //设置非阻塞
    ConBuf* conbuf = new ConBuf;

    conbuf ->fd = Confd;
    conbuf ->ReadFunc = std::bind(&SockServer::ReadFunc, this, conbuf);
    // conbuf ->ReadFunc = std::bind(&SockServer::ReadFunc, this, std::placeholders::_1);
    // conbuf ->WriteFunc = std::bind(&SockServer::WriteFunc, this, std::placeholders::_1);
    conbuf ->readSta = READHEAD;
    conbuf ->readDataLen = HEADLEN;
    conbuf ->readBuf = (char*)&conbuf ->headBuf;
    EpollOper(Confd, READ, ADD, conbuf);
}

void SockServer::ListenWork(void)
{
    sigset_t sa;
    sigfillset(&sa);
    pthread_sigmask(SIG_BLOCK, &sa, nullptr);
    while(true)
    {
        int ret = epoll_wait(epFd, evBuf, MAXCON, -1);
        if(ret < 0)
        {
            std::cout << strerror(errno) << std::endl;
            continue;
        }
        // std::cout << "aaa" << std::endl;
        for(int i = 0; i < ret; i++)
        {
            ConBuf* task = static_cast<ConBuf*>(evBuf[i].data.ptr);
            if(evBuf[i].events == EPOLLIN)
            {
                threadPool ->AddTask(task ->ReadFunc);  //添加读事件到IO线程池中
                // task->ReadFunc(task);
            }
            else if(evBuf[i].events == EPOLLOUT)        //暂时用不到写事件
            {
                threadPool ->AddTask(task ->WriteFunc);
                // task->WriteFunc(task);
            }
        }
    }
}

