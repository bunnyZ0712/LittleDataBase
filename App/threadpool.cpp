#include <chrono>
#include <iostream>
#include <signal.h>

#include "threadpool.h"

#define OnceCreateNum   2
#define OnceDestroyNum  2

CThread* CThread::m_instance = nullptr;

CThread::CThread(int Tmin, int Tmax)
{
    if(Tmin <= Tmax)
    {
        minNum = Tmin;
        maxNum = Tmax;
        busyNum = 0;
        liveNum = Tmin;
        shutdown = false;
        exitNum = 0;
        if(Tmin < Tmax)
        {
            Manager = std::thread(ManagerThread, this);
            Recycle = std::thread(RecycleThread, this);
        }
        PackBuf.reserve(Tmax);
        ThreadBuf.reserve(Tmax);
        WriteTaskQueue = LockFreeQueue<WriteTask>::GetInstance();
        for(int i = 0; i < Tmin; i++)
        {
            PackBuf.emplace_back(std::packaged_task<void(void*)>(WorkThread));
            ThreadBuf.emplace_back(PackBuf.back().get_future(), std::thread(ref(PackBuf.back()), this));
        }
    }
}

CThread::~CThread()
{
    shutdown = true;
    for(int i = 0; i < liveNum; i++)
    {
        NotEmpty.notify_one();
    }

    for(auto& it : ThreadBuf)
    {
        it.fut.get();
        it.work.join();
    }
    ThreadBuf.clear();
    PackBuf.clear();

    if(minNum < maxNum)
    {
        Manager.join();
        Recycle.join();
    }
}

CThread::Destroy_CT::~Destroy_CT()
{
    if(CThread::m_instance != nullptr)
    {
        delete CThread::m_instance;
        CThread::m_instance == nullptr;
    }
}

void CThread::Create(int Tmin, int Tmax)
{
    if(CThread::m_instance == nullptr)
    {
        CThread::m_instance = new CThread(Tmin, Tmax);
        static Destroy_CT DCT;
    }
}

CThread* CThread::GetInstance(int Tmin, int Tmax)
{
    static std::once_flag create_flag;

    std::call_once(create_flag, Create, Tmin, Tmax);

    return CThread::m_instance;
}

CThread* CThread::GetInstance(void)
{
    return CThread::m_instance;
}

size_t CThread::GetTaskNum(void)
{
    std::unique_lock<std::mutex> TQlock(TQMutex);

    size_t tasknum = TaskBuf.size();

    return tasknum;
}

bool CThread::GetTask(Task& task)
{
    std::unique_lock<std::mutex> TQlock(TQMutex);
    if(TaskBuf.empty())
    {
        return false;
    }
    task = TaskBuf.front();
    TaskBuf.pop();

    return true;
}

void CThread::AddTask(const Task& task)
{
    std::unique_lock<std::mutex> TQlock(TQMutex);

    TaskBuf.push(task);

    NotEmpty.notify_one();
}

void CThread::WorkThread(void* arg)
{
    CThread* ct = static_cast<CThread*>(arg);
    Task readtask;
    WriteTask writetask;
    sigset_t sa;
    sigfillset(&sa);
    pthread_sigmask(SIG_BLOCK, &sa, nullptr);
    auto condition = [=](void) ->bool{
        if((ct ->GetTaskNum() > 0) || (ct ->shutdown == true) || (ct ->exitNum > 0) || ct ->WriteTaskQueue ->size() > 0)
            return true;
        else
            return false;
    };
    while(true)
    {
        std::unique_lock<std::mutex> CTlock(ct ->CThreadMutex);
        ct ->NotEmpty.wait(CTlock, condition);

        if(ct ->shutdown == true)
        {
            std::cout << "thread exit" << std::endl;
            return;
        }
        if(ct ->exitNum > 0)
        {
            ct ->exitNum--;
            ct ->liveNum--;
            std::cout << "thread exit" << std::endl;
            return;            
        }
        while(ct ->WriteTaskQueue ->Pop(writetask) == true)
        {
            writetask(nullptr, nullptr, 0);
        } 
        if(ct ->GetTaskNum() > 0)
        {
            if(ct ->GetTask(readtask) == false)
            {
                std::cout << "Get Task failed" << std::endl;
                continue;
            }   
            ct ->busyNum++;
            CTlock.unlock();
            readtask(nullptr);
            CTlock.lock();
            ct ->busyNum--;         
        }
    }
}

void CThread::ManagerThread(void* arg)
{
    CThread* ct = static_cast<CThread*>(arg);
    int busy, live, tasknum;
    std::chrono::seconds stm(5);
    sigset_t sa;
    sigfillset(&sa);
    pthread_sigmask(SIG_BLOCK, &sa, nullptr);
    while(true)
    {
        std::this_thread::sleep_for(stm);
        if(ct ->shutdown == true)
        {
            return;
        }
        std::unique_lock<std::mutex> CTlock(ct ->CThreadMutex);
        busy = ct ->busyNum;
        live = ct ->liveNum;
        tasknum = ct ->GetTaskNum();
        CTlock.unlock();

        if((live < ct ->maxNum) && (tasknum != 0))
        {
            CTlock.lock();
            int count = live + OnceCreateNum;
            if(count <= ct ->maxNum)
            {
                for(int i = 0; i < OnceCreateNum; i++)
                {
                    ct ->PackBuf.emplace_back(std::packaged_task<void(void*)>(WorkThread));
                    ct ->ThreadBuf.emplace_back(ct ->PackBuf.back().get_future(), std::thread(ref(ct ->PackBuf.back()), ct)); 
                    std::cout << "create thread" << std::endl; 
                    ct ->liveNum++;                  
                }
            }
            else
            {
                for(int i = 0; i < (ct ->maxNum - live); i++)
                {
                    ct ->PackBuf.emplace_back(std::packaged_task<void(void*)>(WorkThread));
                    ct ->ThreadBuf.emplace_back(ct ->PackBuf.back().get_future(), std::thread(ref(ct ->PackBuf.back()), ct));  
                    std::cout << "create thread" << std::endl;  
                    ct ->liveNum++;                 
                }                
            }
        }
        else if((live > ct ->minNum) && (busy * 2 < live))
        {
            CTlock.lock();
            int count = live - OnceDestroyNum;
            if(count >= ct ->minNum)
            {
                ct ->exitNum = OnceDestroyNum;
                for(int i = 0; i < OnceDestroyNum; i++)
                {
                    ct ->NotEmpty.notify_one();
                }
            }
            else
            {
                ct ->exitNum = live - ct ->minNum;
                for(int i = 0; i < (live - ct ->minNum); i++)
                {
                    ct ->NotEmpty.notify_one();
                }
            }
        }
    }
}

void CThread::RecycleThread(void* arg)
{
    CThread* ct = static_cast<CThread*>(arg);
    std::chrono::seconds stm(10);
    std::chrono::microseconds wtm(1);
    sigset_t sa;
    sigfillset(&sa);
    pthread_sigmask(SIG_BLOCK, &sa, nullptr);
    while(true)
    {
        std::this_thread::sleep_for(stm);
        if(ct ->shutdown == true)
        {
            return;
        }
        std::unique_lock<std::mutex> CTlock(ct ->CThreadMutex);

        auto Pit = ct ->PackBuf.begin();
        for(auto Tit = ct ->ThreadBuf.begin(); Tit != ct ->ThreadBuf.end();)
        {
            std::future_status sta = Tit ->fut.wait_for(wtm);
            if(sta == std::future_status::ready)
            {
                Tit ->fut.get();
                Tit ->work.join();
                Pit = ct ->PackBuf.erase(Pit);
                Tit = ct ->ThreadBuf.erase(Tit);
            }
            else
            {
                Tit++;
                Pit++;
            }
        }
    }
}

void CThread::AddWriteTask(const WriteTask& task)
{
    WriteTaskQueue ->Push(task);

    NotEmpty.notify_one();
}