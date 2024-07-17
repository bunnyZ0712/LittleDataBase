#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <queue>
#include <future>
#include <vector>
#include <sys/uio.h>
#include <memory>

#include "LockFreeQueue.h"

using Task = std::function<void(void*)>;
using WriteTask = std::function<void(void*, iovec*, int)>;

struct Thread_Info{
    Thread_Info(std::future<void>&& f, std::thread&& w)
    {
        this ->work = std::forward<std::thread>(w);
        this ->fut = std::forward<std::future<void>>(f);
    }
    std::thread work;
    std::future<void> fut;
};

class CThread{
private:
    CThread(int Tmin, int Tmax);
    ~CThread();

private:
    static CThread* m_instance;
    class Destroy_CT{
    public:
        ~Destroy_CT();
    };
    static void Create(int Tmin, int Tmax);
    static void WorkThread(void* arg);
    static void ManagerThread(void* arg);
    static void RecycleThread(void* arg);
    size_t GetTaskNum(void);
    bool GetTask(Task& task);

public:
    static CThread* GetInstance(int Tmin, int Tmax);
    static CThread* GetInstance(void);
    void AddTask(const Task& task);

    void AddWriteTask(const WriteTask& task);
 
private:
    int minNum;
    int maxNum;
    int busyNum;
    int liveNum;
    int exitNum;
    bool shutdown;

    std::thread Manager;
    std::thread Recycle;
    std::vector<std::packaged_task<void(void*)>> PackBuf;
    std::vector<Thread_Info> ThreadBuf;

    std::queue<Task> TaskBuf;
    std::shared_ptr<LockFreeQueue<WriteTask>> WriteTaskQueue; //写事件无锁队列       IO线程负责发送
    std::mutex CThreadMutex;
    std::mutex TQMutex;
    std::condition_variable NotEmpty;
};


#endif