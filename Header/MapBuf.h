#ifndef MAPBUF_H
#define MAPBUF_H

#include <iostream>
#include <map>
#include <string>

#include "BaseBuf.h"

#include <functional>
#include <map>

std::ostream& operator << (std::ostream& o, const std::map<std::string, std::string>& buf);

class MapBuf : public BaseBuf{
public:
    MapBuf(int Level);
    ~MapBuf();

    virtual bool Insert(void* data, char oper) override;
    virtual bool Delete(void* data, char oper) override;
    virtual bool Get(void* key, void* value, char oper) override;
    virtual void Display(void) override;
    virtual void Save(std::ostream& saveFile) override;

private:
    bool MSet(std::string key, std::string value);
    bool MGet(std::string key, std::string& value);
    bool MDel(std::string keys);
    bool MSetS(std::string key, std::string value);
    bool MGetS(std::string key, std::string& value);
    bool MDelS(std::string key);
    void AnalysisKV(std::string& str, std::map<std::string, std::string>& ans); //解析键值对
    void AnalysisK(std::string& str, std::vector<std::string>& ans);            //解析键值

private:
    SkipList<std::string, std::map<std::string, std::string>> Buf;
    std::map<char, std::function<bool(std::string, std::string)>> SetFunc;
    std::map<char, std::function<bool(std::string, std::string&)>> GetFunc;
    std::map<char, std::function<bool(std::string)>> DelFunc;
};



#endif