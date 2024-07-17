#include "MapBuf.h"
#include <cstring>

std::ostream& operator << (std::ostream& o, const std::map<std::string, std::string>& buf)
{
    o << '{';
    for(auto it : buf)
    {
        o << it.first << ':' << it.second << " ";
    }
    o << '}';

    return o;
}

void MapBuf::AnalysisKV(std::string& str, std::map<std::string, std::string>& ans)      //将命令中的KV解析
{
    int left = str.find_first_not_of("{ ");
    int n = str.size();
    std::vector<std::string> temp;
    while(true)
    {
        int right = str.find_first_of(" }", left);
        if(right == left || right < 0)
            break;
        temp.push_back(str.substr(left, right - left));
        left = str.find_first_not_of(" ", right);
    }
    for(auto it : temp)
    {
        int p = it.find(':');
        ans[it.substr(0, p)] = it.substr(p + 1);
    }
}

void MapBuf::AnalysisK(std::string& str, std::vector<std::string>& ans) //将命令中的K解析
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

void MapBuf::Save(std::ostream& saveFile)
{

}

MapBuf::MapBuf(int Level) : Buf(Level)
{
    SetFunc[OperCode::MSET] = bind(&MapBuf::MSet, this, std::placeholders::_1, std::placeholders::_2);
    SetFunc[OperCode::MSETS] = bind(&MapBuf::MSetS, this, std::placeholders::_1, std::placeholders::_2);
    GetFunc[OperCode::MGET] = bind(&MapBuf::MGet, this, std::placeholders::_1, std::placeholders::_2);
    GetFunc[OperCode::MGETS] = bind(&MapBuf::MGetS, this, std::placeholders::_1, std::placeholders::_2);
    DelFunc[OperCode::MDEL] = bind(&MapBuf::MDel, this, std::placeholders::_1);
    DelFunc[OperCode::MDELS] = bind(&MapBuf::MDelS, this, std::placeholders::_1);
}

MapBuf::~MapBuf()
{

}

bool MapBuf::Insert(void* data, char oper)
{
    char* Data = static_cast<char*>(data);

    int p = GetFirstColon(Data);
    return SetFunc[oper]({Data, Data + p}, {Data + p + 1, Data + strlen(Data)});
}

bool MapBuf::Delete(void* data, char oper)
{
    char* Data = static_cast<char*>(data);

    return DelFunc[oper](Data);
}

bool MapBuf::Get(void* key, void* value, char oper)
{
    char* Data = static_cast<char*>(key);

    return GetFunc[oper](Data, *(std::string*)value);
}

void MapBuf::Display(void)
{
    Buf.DisplayList();
}

bool MapBuf::MSet(std::string key, std::string value)
{
    std::map<std::string, std::string> ans;

    AnalysisKV(value, ans);
    auto ret = Buf.InsertElement(key, {ans.begin(), ans.end()});       //不存在键值，直接插入
    if(ret == true)
        return ret;

    bool findFlag;
    auto& it = Buf.ModifyValue(key, findFlag);      //存在的话，先找出再进行修改
    it[ans.begin() ->first] = ans.begin() ->second;

    return true;
}

bool MapBuf::MSetS(std::string key, std::string value)
{
    std::map<std::string, std::string> ans;
    
    AnalysisKV(value, ans);
    auto ret = Buf.InsertElement(key, ans);     //键值不存在直接插入
    if(ret == true)
        return ret;

    bool findFlag;
    auto& it = Buf.ModifyValue(key, findFlag);    //键值存在，对已有元素值进行更改，没有的元素值进行插入

    for(auto& para : ans)
    {
        it[para.first] = para.second;
    }    

    return true;
}

bool MapBuf::MDel(std::string keys)
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

bool MapBuf::MDelS(std::string key)
{
    return Buf.DeleteElement(key);
}

bool MapBuf::MGet(std::string key, std::string& value)
{
    std::vector<std::string> ans;
    AnalysisK(key, ans); 
    std::map<std::string, std::string> temp;  
    int ret = Buf.GetElement(ans[0], temp);     //是否找到对应键值
    if(ret == false)
        return false;
    
    for(int i = 1; i <ans.size(); i++)      //找到再从键值中寻找对应元素
    {
        if(temp.find(ans[i]) != temp.end())
        {
            value += (ans[i] + ":" + temp[ans[i]] + " ");
        }
    }

    return true;
}

bool MapBuf::MGetS(std::string key, std::string& value)
{
    std::map<std::string, std::string> temp;
    int ret = Buf.GetElement(key, temp);
    if(ret == false)
        return false;

    for(auto& it : temp)
    {
        value += (it.first + ":" + it.second + " ");
    }

    return true;
}

