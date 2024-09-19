#ifndef __SYLAR_SOCKET_H__
#define __SYLAR_SOCKET_H__

#include <memory>
#include "address.h"
#include "noncopyable.h"

namespace sylar{
class Socket: public std::enable_shared_from_this<Socket>, NonCopyable{
public:
    typedef std::shared_ptr<Socket> ptr;
    typedef std::weak_ptr<Socket> weak_ptr;

    Socket(int family,int type, int protocol = 0);
    ~Socket();

    int64_t getSendTimeout();
    void setSendTimeout(int64_t v);

    int64_t getRecvTimeout();
    void setRecvTimeout(int64_t v);

    bool getOption(int level, int option, void* result,size_t* len);
    template<class T>
    bool getOption(int level,int option,T& result){//获取socket选项
        size_t length=sizeof(T);
        return getOption(level,option,&result,&length);

    }

    bool setOption(int level, int option,const void* result, size_t len);
    template<class T>
    bool setOption(int level, int option, const T& value){
        return setOption(level,option,&value,sizeof(T));
    }

    Socket::ptr accept();

    bool init(int sock);

    bool bind(const Address::ptr addr); //绑定地址
    bool connect(const Address::ptr addr,uint64_t timeout = -1); //连接地址
    bool listen(int backlog=SOMAXCONN); //监听
    bool close();

    int send(const void* buffer, size_t length, int flags=0);//发送数据
    int send(const iovec*buffers,size_t length,int flags=0);//iovec一组分散的内存缓冲区的结构
    int sendTo(const iovec* buffers,size_t length,const Address::ptr to,int flags=0);
    int sendTo(const void* buffer, size_t length,const Address::ptr to, int flags=0);


    int recv(void* buffer, size_t length, int flags=0);//接收数据
    int recv(iovec* buffers, size_t length, int flags=0);
    int recvFrom(iovec* buffer, size_t length, Address::ptr from, int flags=0);
    int recvFrom(void* buffer, size_t length, Address::ptr from, int flags=0);

    Address::ptr getRemoteAddress();
    Address::ptr getLocalAddress();

    int getFamily() const{return m_family;};
    int getType() const {return m_type;};
    int getProtocol() const {return m_protocol;};

    bool isConnected() const {return m_isConnected;};
    bool isValid() const {};
    int  getError();

    std::ostream& dump(std::ostream &os) const;
    int getSocket() const {return m_sock;}

    bool cancelRead();
    bool cancelWrite();
    bool cancelAccept();

private:
    void initSock();
    void newSock();
private:
    int m_sock;//socket描述符
    int m_family;//协议族
    int m_type;//socket类型
    int m_protocol; //协议
    bool m_isConnected;

    Address::ptr m_localAddress;//本地的地址
    Address::ptr m_remoteAddress;//连接的地址



};

}

#endif