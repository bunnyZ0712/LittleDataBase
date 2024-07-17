#ifndef LOCKFREEQUEUe_H
#define LOCKFREEQUEUe_H

#include <iostream>
#include <array>
#include <memory>
#include <atomic>
#include <mutex>
#include <thread>

#define QSIZE 2000000

template<typename T>
class LockFreeQueue : public std::enable_shared_from_this<LockFreeQueue<T>>{
public:
private:
    LockFreeQueue(void);
    ~LockFreeQueue();
private:
    static std::shared_ptr<LockFreeQueue<T>> mInstance;
    static void Deleter(LockFreeQueue<T>* ptr);
    static void Create(void);
    int ConvertIndex(int index);
public:
    static std::shared_ptr<LockFreeQueue<T>> GetInstance(void);
    int size(void);
    bool Push(T data);
    bool Pop(T& task); 
private:
    volatile std::atomic<int> mWriteIndex;   //当前可写入位置 
    volatile std::atomic<int> mReadIndex;   //当前可读取位置
    volatile std::atomic<int> mReadMax;     //当前最大读取位置
    volatile std::atomic<int> mCount;       //任务数
    std::array<T, QSIZE> mBuffer;
};

template<typename T>
std::shared_ptr<LockFreeQueue<T>> LockFreeQueue<T>::mInstance = nullptr;

template<typename T>
LockFreeQueue<T>::LockFreeQueue(void)
{
    mWriteIndex = 0;
    mReadIndex = 0;
    mReadMax = 0;
    mCount = 0;
}

template<typename T>
LockFreeQueue<T>::~LockFreeQueue()
{

}

template<typename T>
void LockFreeQueue<T>::Deleter(LockFreeQueue<T>* ptr)
{
    delete ptr;
}

template<typename T>
void LockFreeQueue<T>::Create(void)
{
    if(mInstance == nullptr)
    {
        mInstance.reset(new LockFreeQueue<T>, Deleter);
    }
}

template<typename T>
std::shared_ptr<LockFreeQueue<T>> LockFreeQueue<T>::GetInstance(void)
{
    std::once_flag createFlag;

    std::call_once(createFlag, Create);

    return mInstance ->shared_from_this();
}

template<typename T>
int LockFreeQueue<T>::ConvertIndex(int index)
{
    return index % QSIZE;
}

template<typename T>
int LockFreeQueue<T>::size(void)
{
    return mCount.load();
}

template<typename T>
bool LockFreeQueue<T>::Push(T data)
{
    int currentWriteIndex;
    int currentReadIndex;

    do{ 
        currentWriteIndex = mWriteIndex.load();
        currentReadIndex = mReadIndex.load();
        if(ConvertIndex(currentWriteIndex + 1) == currentReadIndex) //队列满了
        {
            return false;
        } 
    }while(!mWriteIndex.compare_exchange_weak(currentWriteIndex, ConvertIndex(currentWriteIndex + 1)));
    mBuffer[currentWriteIndex] = data;
    while(!mReadMax.compare_exchange_weak(currentWriteIndex, ConvertIndex(currentWriteIndex + 1)))
    {
        std::this_thread::yield();      //多线程竞争时，等待排在前面的更新数据，因为只是等待更新，所以不需要阻塞，只需让出cpu，但还是就绪态
    }
    mCount++;
    
    return true;
}

template<typename T>
bool LockFreeQueue<T>::Pop(T& task)
{
    int currentReadMax;
    int currentReadIndex;

    do{
        currentReadIndex = mReadIndex.load();
        currentReadMax = mReadMax.load();
        if(ConvertIndex(currentReadIndex) == ConvertIndex(currentReadMax))  //队列为空
        {
            return false;
        }
        task = mBuffer[currentReadIndex];
        if(mReadIndex.compare_exchange_weak(currentReadIndex, ConvertIndex(currentReadIndex + 1)))
        {
            mCount--;
            // std::cout << mCount << std::endl;
            return true;
        }
    }while(true);  
}


#endif