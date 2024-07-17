#include "SetBuf.h"

#include <cstring>

std::ostream& operator << (std::ostream& o, const std::set<std::string>& buf)
{
    o << '{';
    for(auto it : buf)
    {
        o << it << " ";
    }
    o << '}';

    return o;
}

void SetBuf::AnalysisK(std::string& str, std::vector<std::string>& ans) //将命令中的K解析
{
    int left = str.find_first_of(" :");
    ans.push_back(str.substr(0, left));
    left = str.find_first_not_of("{ :", left + 1);
    int n = str.size();
    while(true)
    {
        int right = str.find_first_of(" }", left);
        if(right == left || right < 0)
            break;
        ans.push_back(str.substr(left, right - left));
        left = str.find_first_not_of(" ", right);
    }    
}

void SetBuf::Save(std::ostream& saveFile)
{

}

SetBuf::SetBuf(int Level) : Buf(Level)
{
    SetFunc[OperCode::SSET] = bind(&SetBuf::SSet, this, std::placeholders::_1);
    SetFunc[OperCode::SSETS] = bind(&SetBuf::SSetS, this, std::placeholders::_1);
    GetFunc[OperCode::SGET] = bind(&SetBuf::SGet, this, std::placeholders::_1, std::placeholders::_2);
    GetFunc[OperCode::SGETS] = bind(&SetBuf::SGetS, this, std::placeholders::_1, std::placeholders::_2);
    DelFunc[OperCode::SDEL] = bind(&SetBuf::SDel, this, std::placeholders::_1);
    DelFunc[OperCode::SDELS] = bind(&SetBuf::SDelS, this, std::placeholders::_1);
}

SetBuf::~SetBuf()
{

}

bool SetBuf::Insert(void* data, char oper)
{
    char* Data = static_cast<char*>(data);

    return SetFunc[oper](Data);
}

bool SetBuf::Delete(void* data, char oper)
{
    char* Data = static_cast<char*>(data);

    return DelFunc[oper](Data);
}

bool SetBuf::Get(void* key, void* value, char oper)
{
    char* Data = static_cast<char*>(key);

    return GetFunc[oper](Data, *(std::string*)value);
}

void SetBuf::Display(void)
{
    Buf.DisplayList();
}

bool SetBuf::SSet(std::string kv)
{
    std::vector<std::string> ans;
    
    AnalysisK(kv, ans);
    auto ret = Buf.InsertElement(ans[0], {ans.begin() + 1, ans.end()});        //键值不存在就插入
    if(ret == true)
        return true;

    bool findFlag;
    auto& it = Buf.ModifyValue(ans[0], findFlag);  //键值存在，就在原有集合进行修改
    it.insert(ans.begin() + 1, ans.end());

    return true;
} 

bool SetBuf::SSetS(std::string kv)
{
    std::vector<std::string> ans;

    AnalysisK(kv, ans);
    for(auto& it : ans)
    {
        std::cout << it << std::endl;
    }
    auto ret = Buf.InsertElement(ans[0], {ans.begin() + 1, ans.end()});        //键值不存在就插入
    if(ret == true)
        return true;

    bool findFlag;
    auto& it = Buf.ModifyValue(ans[0], findFlag);  //键值存在，就在原有集合进行修改
    it.insert(ans.begin() + 1, ans.end());

    return true;
}

bool SetBuf::SDel(std::string keys)
{
    std::vector<std::string> ans;
    AnalysisK(keys, ans);

    bool findFlag;
    auto& it = Buf.ModifyValue(ans[0], findFlag);   //首先判断键值是否存在
    if(findFlag == false)
        return false;
    for(int i = 1; i < ans.size(); i++)
    {
        it.erase(ans[i]);
    }

    return true;
}

bool SetBuf::SDelS(std::string key)
{
    return Buf.DeleteElement(key);
}

bool SetBuf::SGet(std::string key, std::string& value)
{
    std::vector<std::string> ans;
    AnalysisK(key, ans);
    std::set<std::string> temp;
    int ret = Buf.GetElement(ans[0], temp);    
    if(ret == false)
        return false;
    if(temp.find(ans[1]) == temp.end())
    {
        return false;
    }
    return true;
}

bool SetBuf::SGetS(std::string key, std::string& value)
{
    std::set<std::string> temp;
    int ret = Buf.GetElement(key, temp);
    if(ret == false)
        return false;

    for(auto& it : temp)
    {
        value += (it + " ");
    }

    return true;
}