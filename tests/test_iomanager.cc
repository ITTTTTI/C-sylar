#include "sylar/sylar.h"
#include "sylar/iomanager.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <unistd.h>

sylar::Logger::ptr g_logger =SYLAR_LOG_ROOT();
int sock=0;
void test_fiber(){
   SYLAR_LOG_INFO(g_logger)<<"test_fiber="<<sock;

   sock =socket(AF_INET,SOCK_STREAM,0);
     /*设置套接字选项的函数
      int socket(int domain, int type, int protocol);
      domain：指定地址族（Address Family），它决定了socket使用的协议。最常用的值有AF_INET（IPv4）和AF_INET6（IPv6）。
      type：指定socket的类型。最常见的类型是SOCK_STREAM（用于TCP连接）和SOCK_DGRAM（用于UDP数据报）。
      protocol：通常设置为0，让系统选择默认的协议。*/
    fcntl(sock,F_SETFL,O_NONBLOCK);//file control 对文件描述符进行各种操作

    sockaddr_in addr;//sockaddr_in 是 C 和 C++ 中用于表示 IPv4 地址和端口号的一个结构体

    /*struct sockaddr_in {
    sa_family_t sin_family; // 地址族，对于 IPv4 来说，它总是 AF_INET
    uint16_t sin_port;      // 端口号，网络字节序
    struct in_addr sin_addr; // IPv4 地址
    // 注意：有些系统或编译器可能会添加一个额外的 char sin_zero[8]; 用于填充到结构体大小，以匹配 sockaddr 结构体的大小
};*/
    memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_port=htons(80);
    inet_pton(AF_INET,"110.242.68.66", &addr.sin_addr.s_addr);
    //用于将点分十进制格式的 IP 地址（IPv4 或 IPv6）转换为网络字节序的二进制形式
    

    if(!connect(sock, (const sockaddr*)&addr,sizeof(addr))){
        
        //connect函数是一个用于创建与指定套接字的连接的函数

    }
    else if(errno==EINPROGRESS){

        SYLAR_LOG_INFO(g_logger)<<"add event errno="<<errno <<" "<<strerror(errno);
        sylar::IOManager::GetThis()->addEvent(sock,sylar::IOManager::READ,[](){
        SYLAR_LOG_INFO(g_logger)<<"read callback";});
        sylar::IOManager::GetThis()->addEvent(sock,sylar::IOManager::WRITE,[](){
                   SYLAR_LOG_INFO(g_logger)<<"write callback";
                   sylar::IOManager::GetThis()->cancelEvent(sock,sylar::IOManager::READ);
                   close(sock);
        });
    }else{
        SYLAR_LOG_INFO(g_logger)<<"else"<<errno<<" "<<strerror(errno);
    }
}

void test1(){
    sylar::IOManager iom(2);
    iom.schedule(&test_fiber);
}
sylar::Timer::ptr timer;
void test_timer(){
    sylar::IOManager iom(2);
    timer=iom.addTimer(1000,[](){
        static int i=0;
        SYLAR_LOG_INFO(g_logger)<<"hello timer i="<<i;    
        if(++i==3){
        //timer->cancel();
        timer->reset(2000,true);
        }
    },true);
    

}

int main(int argc, char** argv){
    //test1();
    //return 0;
    test_timer();
    return 0;
}