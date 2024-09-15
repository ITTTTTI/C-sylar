#ifndef __FD_MANAGER_H__
#define __FD_MANAGER_H__

#include <memory>
#include "thread.h"
#include "iomanager.h"
#include "singleton.h"

namespace sylar{

class FdCtx: public std::enable_shared_from_this<FdCtx> {
public:
    typedef std::shared_ptr<FdCtx> ptr;
    FdCtx(int fd); //构造函数，初始化文件描述符 fd。
    ~FdCtx();
    
    bool init();//初始化函数，用于初始化文件描述符相关的设置
    bool isInit() const{return m_isInit;} //判断该文件描述符是否已初始化
    bool isSocket()const{return m_isSocket;}
    bool isClosed() const{return m_isClosed;}
    bool close();

    void setUserNonblock(bool v){m_userNonblock =v;}
    bool getUserNonblock() const{return m_userNonblock;}

    void setsysNonblock(bool v) {m_sysNonblock = v;}
    bool getsysNonblock() const{return m_sysNonblock;}

    void setTimeout(int type, uint64_t v);
    uint64_t getTimeout(int type);

private:
    bool m_isInit:1;
    bool m_isSocket:1;
    bool m_sysNonblock:1;
    bool m_userNonblock:1;
    bool m_isClosed:1;
    int m_fd;
    uint64_t m_recvTimeout;
    uint64_t m_sendTimeout;
    sylar::IOManager* m_iomanager;  
};

//FdManager 类用于管理多个 FdCtx 实例，
//通过文件描述符索引 FdCtx 的上下文对象，常用于高并发场景下管理大量文件描述符。
class FdManager{
public:
    typedef RWMutex RWMutexType;

    FdManager();

    FdCtx::ptr get(int fd ,bool auto_create = false);
    void del(int fd);

private:
    RWMutexType m_mutex;
    std::vector<FdCtx::ptr> m_datas;
};

typedef Singleton<FdManager> FdMgr;

}



#endif
