#ifndef STRINGBUF_H
#define STRINGBUF_H

#include "BaseBuf.h"

// enum StringBufCode{
//     SET = 0,
//     GET,
//     DEL
// }; 

class StringBuf : public BaseBuf{
public:
    StringBuf(int Level);
    ~StringBuf();

    virtual bool Insert(void* data, char oper) override;
    virtual bool Delete(void* data, char oper) override;
    virtual bool Get(void* key, void* value, char oper) override;
    virtual void Display(void) override;
    virtual void Save(std::ostream& saveFile) override;

    bool Set(std::string key, std::string value);
    bool Get(std::string key, std::string& value);
    bool Del(std::string key);

private:
    SkipList<std::string, std::string> Buf;
};


#endif