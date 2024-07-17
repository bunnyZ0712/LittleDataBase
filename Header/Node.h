#ifndef NODE_H
#define NODE_H

template<typename K, typename V>
class Node{
public:
    Node();
    Node(K k, V v, int level) noexcept;
    ~Node() noexcept;

    K GetKey(void) const;
    V& GetValue(void);
    void SetValue(V v);

    Node<K, V>** forward;   //每级跳变指向的下一级元素，存储的是指针，所以为指针数组
    int nodeLevel;      //总共有多少级
private:
    K key;
    V value;
};

template<typename K, typename V>
Node<K, V>::Node()
{

}

template<typename K, typename V>
Node<K, V>::Node(K k, V v, int level) noexcept
{
    this ->key = k;
    this ->value = v;
    this ->nodeLevel = level;

    this ->forward = new Node<K, V>*[level + 1];

    for(int i = 0; i <= level; i++)
    {
        forward[i] = nullptr;
    }
}

template<typename K, typename V>
Node<K, V>::~Node() noexcept
{
    delete[] this ->forward;
}

template<typename K, typename V>
void Node<K, V>::SetValue(V v)
{
    this ->value = v;
}

template<typename K, typename V>
K Node<K, V>::GetKey(void) const
{
    return this ->key;
}

template<typename K, typename V>
V& Node<K, V>::GetValue(void)
{
    return this ->value;
}

#endif