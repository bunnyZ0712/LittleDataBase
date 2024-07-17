#ifndef BASEBUF_H
#define BASEBUF_H

#include "common.h"
#include "SkipList.h"

class BaseBuf{
public:
    BaseBuf() = default;
    virtual ~BaseBuf(){};

    virtual bool Insert(void* data, char oper) = 0;
    virtual bool Delete(void* data, char oper) = 0;
    virtual bool Get(void* key, void* value, char oper) = 0;
    virtual void Display(void) = 0;
    virtual void Save(std::ostream& saveFile) = 0;
};

int GetFirstColon(const char* buf);

#endif