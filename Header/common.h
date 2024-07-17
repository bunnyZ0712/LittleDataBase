#ifndef COMMON_H
#define COMMON_H

#include <functional>

#define HEADLEN sizeof(InfoHead)

using CallBackFunc = std::function<void(void*)>;

enum ReadSta{       //读取状态
    READHEAD = 0,
    READDATA,
    READOVER
};

enum EpOper{    //EPOLL操作
    READ = 0,
    WRITE,
    ADD,
    DELETE,
    MOD
};

enum OperCode{              //操作码
    INSERTOPER = 0,     //插入
    SET,
    MSET,
    MSETS,
    SSET,
    SSETS,

    DELETEOPER,         //删除
    DEL,
    MDEL,
    MDELS,
    SDEL,
    SDELS,

    QUERYOPER,           //查询
    GET,
    MGET,
    MGETS,
    SGET,
    SGETS,

    BGSAVE
};

enum OperStatus{    //操作状态
    INSERTOK = 0,
    INSERTEXIST,
    DELETOK,
    DELETEERROR,
    GETOK,
    NODATA
};

enum OperBufType{    //操作状态
    STRING = 0,
    MAP,
    SETS
};

#pragma pack(1)
struct InfoHead{ //包头
    char Oper;      //操作标识
    char OperBuf;   //操作容器
    int  DataLen;   //数据长度
    char ComdNum;   //命令个数
};

#pragma pack()

struct ConBuf{
    int fd;
    InfoHead headBuf;       //包头
    CallBackFunc ReadFunc;
    CallBackFunc WriteFunc;
    int readDataLen;    //需要读取的数据长度
    char* readBuf;      //指向数据存储的空间
    char* readMemPointer;   //指向保存数据的空间
    int writeDataLen;
    char* writeBuf;
    char* writeMemPointer;
    ReadSta readSta;
};


#endif