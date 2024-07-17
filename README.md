![image](https://github.com/user-attachments/assets/24fc086a-2107-433e-9d6c-6b5f5bf1fc38)# LittleDataBase
轻量级缓存数据库，底层使用跳表实现，仿照Redis线程模型


支持STRING、MAP、SET三种结构

bgsave:保存RDB快照

string:
  SET:插入数据
  GET:获取数据
  DEL:删除数据

MAP:
  MSET:插入单个数据
  MGET:获取单个数据
  MDEL:删除单个数据
  MSETS:插入新的集合
  MGETS:获取整个集合
  MDELS:删除整个集合

SET:
  SSET:插入单个数据
  SGET:获取单个数据
  SDEL:删除单个数据
  SSETS:插入新的集合
  SGETS:获取整个集合
  SDELS:删除整个集合
