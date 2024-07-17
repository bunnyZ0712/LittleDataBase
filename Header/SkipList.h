#ifndef SKIPLIST_H
#define SKIPLIST_h

#include <iostream>
#include <random>
#include "Node.h"

// enum DataStatus{
//     Insert = 0,
//     EXIST
// };

template<typename K, typename V>
class SkipList{
public:
    SkipList(int maxLevel) noexcept;
    ~SkipList();

    bool InsertElement(const K key, const V value);   //插入新元素
    bool GetElement(const K key, V& v); //获取元素
    bool DeleteElement(const K key);    //删除元素
    V& ModifyValue(const K key, bool& findFlag);   //修改键值对应元素值
    void DisplayList(void); //展示现有元素
    int Size(void);
    Node<K, V>* GetHeader(void);
    
private:
    int GetRandomLevel(void);       //获得随机层级
    void clear(Node<K, V>* cur);    //递归释放所有节点
    Node<K, V>* CreateNode(K k, V v, int level);    //创建新节点

private:
    int maxLevel;       //最大层级 0~maxLevel
    int SkipListLevel;  //目前最大层级
    Node<K, V>* header; //底层头节点

    int elementCount;
};

template<typename K, typename V>
Node<K, V>* SkipList<K, V>::GetHeader(void)
{
    return header;
}

template<typename K, typename V>
SkipList<K, V>::SkipList(int maxLevel) noexcept
{
    this ->maxLevel = maxLevel;
    this ->elementCount = 0;
    this ->SkipListLevel = 0;

    K k;
    V v;
    this ->header = new Node<K, V>(k, v, maxLevel);     //初始化头节点
}

template<typename K, typename V>
SkipList<K, V>::~SkipList()
{
    if(header ->forward[0] != nullptr)
    {
        clear(header ->forward[0]);
    }

    delete header;
}

template<typename K, typename V>
int SkipList<K, V>::Size(void)
{
    return elementCount;
}

template<typename K, typename V>
void SkipList<K, V>::clear(Node<K, V>* cur) //递归删除现有节点
{
    if(cur ->forward[0] != nullptr)
    {
        clear(cur ->forward[0]);
    }

    delete cur;
}

template<typename K, typename V>
Node<K, V>* SkipList<K, V>::CreateNode(K k, V v, int level) //创建节点
{
    Node<K, V>* node = new(std::nothrow) Node<K, V>(k, v, level);

    return node;
}

template<typename K, typename V>
int SkipList<K, V>::GetRandomLevel(void)  //随机获取插入层级
{
    int k = 1;
    // srand((unsigned)time(NULL));

    std::random_device rd; // 非确定性随机数生成器  
    std::mt19937 gen(rd()); // 使用 random_device 来种子 mersenne_twister_engine  
  
    // 创建一个随机数分布  
    // std::uniform_int_distribution<int> 允许你指定一个范围，并在这个范围内生成均匀分布的随机数  
    std::uniform_int_distribution<> dis(1, 100); // 分布范围从1到100  
  
    // 生成随机数  
    // int randomNumber = dis(gen); // 使用引擎gen和分布dis来生成随机数 
    while(dis(gen) % 2)
    {
        k++;
    }

    k = (k < maxLevel) ? k : maxLevel;
    return k;
}

template<typename K, typename V>
bool SkipList<K, V>::GetElement(const K key, V& v)  //查询元素
{
    Node<K, V>* current = this ->header;

    for(int i = SkipListLevel; i >= 0; i--)
    {
        while(current ->forward[i] != nullptr && current ->forward[i] ->GetKey() < key)
        {
            current = current ->forward[i];
        }
    }
    current = current ->forward[0];
    if(current != nullptr && current ->GetKey() == key)
    {
        v = current ->GetValue();
        return true;
    }

    return false;
}

template<typename K, typename V>
V& SkipList<K, V>::ModifyValue(const K key, bool& findFlag) //确定已经有对应键值，才进行修改
{
    Node<K, V>* current = this ->header;
    
    for(int i = SkipListLevel; i >= 0; i--)
    {
        while(current ->forward[i] != nullptr && current ->forward[i] ->GetKey() < key)
        {
            current = current ->forward[i];
        }
    }
    auto temp = current;
    current = current ->forward[0];
    // return current ->GetValue();
    if(current != nullptr && current ->GetKey() == key)
    {
        findFlag = true;
        return current ->GetValue();
    }  

    findFlag = false;
    return temp ->GetValue();
}

template<typename K, typename V>
void SkipList<K, V>::DisplayList(void)
{
    std::cout << "\n*****Skip List*****"<<"\n"; 
    for (int i = 0; i <= SkipListLevel; i++) {
        Node<K, V> *node = this->header->forward[i]; 
        std::cout << "Level " << i << ": ";
        while (node != NULL) 
        {
            std::cout << node->GetKey() << ":" << node->GetValue() << ";";
            node = node->forward[i];
        }
        std::cout << std::endl;
    }    
}

template<typename K, typename V>
bool SkipList<K, V>::InsertElement(const K key, const V value)    //插入元素
{
    Node<K, V>* current = this ->header;
    Node<K, V>* update[maxLevel + 1];       //存储的是每一层刚好小于key值的节点
    for(int i = 0; i <= maxLevel; i++)
    {
        update[i] = nullptr;
    }

    for(int i = SkipListLevel; i >= 0; i--) //找到每一层最大的不大于key值的位置，从高往低找
    {
        while(current ->forward[i] != nullptr && current ->forward[i] ->GetKey() < key) 
        {
            current = current ->forward[i];
        }
        update[i] = current;
    }

    current = current ->forward[0]; //最后一层小于key值，最接近的元素位置
    if(current != nullptr && current ->GetKey() == key) //如果找到对应元素就进行修改
    {
        // current ->value = value;
        // std::cout << "key:" << key << '\t' << "exists" << std::endl;
        // return DataStatus::EXIST;
        return false;
    }

    if(current == nullptr || current ->GetKey() != key)
    {
        int randomLevel = GetRandomLevel();
        // std::cout << "level:" << randomLevel << std::endl;

        if(randomLevel > SkipListLevel)     //如果大于现有层级，需要将新增层级头节点初始化
        {
            for(int i = SkipListLevel + 1; i < randomLevel + 1; i++)
            {
                update[i] = header;
            }
            SkipListLevel = randomLevel;
        }

        Node<K, V>* insertedNode = CreateNode(key, value, randomLevel);

        for(int i = 0; i <= randomLevel; i++)       //将元素在各层之间串联起来
        {
            insertedNode ->forward[i] = update[i] ->forward[i];
            update[i] ->forward[i] = insertedNode;
        }

        // std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl; 
        elementCount++;
    }

    return true;
}

template<typename K, typename V>
bool SkipList<K, V>::DeleteElement(const K key) //删除元素
{
    Node<K, V>* current = this ->header;
    Node<K, V>* update[maxLevel + 1];

    for(int i = SkipListLevel; i >= 0; i--)
    {
        while(current ->forward[i] != nullptr && current ->forward[i] ->GetKey() < key)
        {
            current = current ->forward[i];
        }
        update[i] = current;
    }
    current = current ->forward[0];

    if(current != nullptr && current ->GetKey() == key)
    {
        for(int i = 0; i <= SkipListLevel; i++)
        {
            if (update[i]->forward[i] != current) 
                break;
            update[i] ->forward[i] = current ->forward[i];
        }

        while(SkipListLevel > 0 && this ->header ->forward[SkipListLevel] == nullptr)   //如果该层没有元素，则清理
        {
            SkipListLevel--;
        }

        delete current;
        elementCount--;
        return true;
    }

    return false;
}


#endif