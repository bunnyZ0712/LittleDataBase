#ifndef SETBUF_H
#define SETBUF_H

#include <iostream>
#include <set>
#include <string>

#include "BaseBuf.h"

#include <functional>
#include <map>

std::ostream& operator << (std::ostream& o, const std::map<std::string, std::string>& buf);

class SetBuf : public BaseBuf{
public:
    SetBuf(int Level);
    ~SetBuf();

    virtual bool Insert(void* data, char oper) override;
    virtual bool Delete(void* data, char oper) override;
    virtual bool Get(void* key, void* value, char oper) override;
    virtual void Display(void) override;
    virtual void Save(std::ostream& saveFile) override;

private:
    bool SSet(std::string kv);
    bool SGet(std::string key, std::string& value);
    bool SDel(std::string keys);
    bool SSetS(std::string kv);
    bool SGetS(std::string key, std::string& value);
    bool SDelS(std::string key);
    void AnalysisK(std::string& str, std::vector<std::string>& ans);            //解析键值

private:
    SkipList<std::string, std::set<std::string>> Buf;
    std::map<char, std::function<bool(std::string)>> SetFunc;
    std::map<char, std::function<bool(std::string, std::string&)>> GetFunc;
    std::map<char, std::function<bool(std::string)>> DelFunc;
};

#endif