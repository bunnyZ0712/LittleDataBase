#include "StringBuf.h"
#include <cstring>

inline int GetFirstColon(const char* buf)
{
    int n = strlen(buf);

    for(int i = 0; i < n; i++)
    {
        if(buf[i] == ':')
            return i;
    }

    return -1;
}

StringBuf::StringBuf(int Level) : Buf(Level)
{

}

StringBuf::~StringBuf()
{

}

void StringBuf::Save(std::ostream& saveFile)
{
    saveFile << "STRING:" << Buf.Size() << std::endl;

    auto it = Buf.GetHeader() ->forward[0];

    for(int i = 0; i < Buf.Size(); i++)
    {
        saveFile << it->GetKey() << ":" << it ->GetValue() << std::endl;
        it = it ->forward[0];
    }
}

bool StringBuf::Insert(void* data, char oper)
{
    char* Data = static_cast<char*>(data);

    int p = GetFirstColon(Data);
    return Set({Data, Data + p}, {Data + p + 1, Data + strlen(Data)});
}

bool StringBuf::Delete(void* data, char oper)
{
    char* Data = static_cast<char*>(data);

    return Del(Data);
}

bool StringBuf::Get(void* key, void* value, char oper)
{
    char* Data = static_cast<char*>(key);
    std::string* ans = static_cast<std::string*>(value);

    return Get(Data, *ans);
}

void StringBuf::Display(void)
{
    Buf.DisplayList();
}



bool StringBuf::Set(std::string key, std::string value)
{   
    auto ret = Buf.InsertElement(key, value);   //元素不存在直接插入
    if(ret == true)
        return ret;
    std::string ans;
    bool findFlag;
    Buf.ModifyValue(key, findFlag) = value; //键值存在，进行修改

    return true;
}

bool StringBuf::Get(std::string key, std::string& value)
{
    return Buf.GetElement(key, value);
}

bool StringBuf::Del(std::string key)
{
    return Buf.DeleteElement(key);
}