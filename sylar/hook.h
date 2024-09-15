#ifndef __SYLAR_HOOK_H__
#define __SYLAR_HOOK_H__

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
/*函数钩子是一种技术，通过它可以拦截或替换系统函数调用，
使得我们可以在调用系统函数之前或之后执行一些自定义的操作。*/
namespace sylar
{
    bool is_hook_enabled();
    void set_hook_enabled(bool flag);
}

extern "C"{

//sleep
/*extern "C"是一个链接规范（linkage specification），
它告诉C++编译器，被extern "C"包围的代码应该按照C语言的链接规则来处理。
因为C++支持函数重载（即可以有多个同名函数，只要它们的参数列表不同），而C语言不支持。*/
typedef unsigned int(*sleep_fun)(unsigned int seconds);
extern sleep_fun sleep_f;
/*这两行声明了两个外部变量sleep_f和usleep_f，
它们分别是类型为sleep_fun和usleep_fun的函数指针。这意味着在别的地方，
这两个函数指针会被赋值*/
typedef int (*usleep_fun)(useconds_t usec);
extern usleep_fun usleep_f;

typedef int (*nanosleep_fun)(const struct timespec *req, struct timespec *rem);
extern nanosleep_fun nanosleep_f;

//socket
typedef int (*socket_fun)(int domain, int type, int protocol);
extern socket_fun socket_f;

typedef int (*connect_fun)(int sockfd, const struct sockaddr *addr,socklen_t addrlen);
extern connect_fun connect_f;

typedef int (*accept_fun)(int s, struct sockaddr *addr, socklen_t *addrlen);
extern accept_fun accept_f;

//read
typedef ssize_t (*read_fun)(int fd, void *buf, size_t count);
extern read_fun read_f;

typedef ssize_t (*readv_fun)(int fd, const struct iovec *iov, int iovcnt);
extern readv_fun readv_f;

typedef ssize_t (*recv_fun)(int sockfd, void *buf, size_t len, int flags);
extern recv_fun recv_f;

typedef ssize_t (*recvfrom_fun)(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen);
extern recvfrom_fun recvfrom_f;


typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr *msg, int flags);
extern recvmsg_fun recvmsg_f;

//write
typedef ssize_t (*write_fun)(int fd, const void *buf, size_t count);
extern  write_fun write_f;

typedef ssize_t (*writev_fun)(int fd, const struct iovec *iov, int iovcnt);
extern  writev_fun writev_f;

typedef ssize_t (*send_fun)(int sockfd, const void *buf, size_t len, int flags);
extern  send_fun send_f;

typedef ssize_t (*sendto_fun)(int sockfd, const void *buf, size_t len, int flags,
                        const struct sockaddr *dest_addr, socklen_t addrlen);
extern  sendto_fun sendto_f;


typedef ssize_t (*sendmsg_fun)(int sockfd, const struct msghdr *msg, int flags);
extern  sendmsg_fun sendmsg_f;


//close
typedef int (*close_fun)(int fd);
extern close_fun close_f;

//socket opt
typedef int (*fcntl_fun)(int fd, int cmd,.../*arg*/);
extern fcntl_fun fcntl_f;

typedef int (*ioctl_fun)(int fd, unsigned long request, ...);
extern ioctl_fun ioctl_f;

typedef int (*getsockopt_fun)(int sockfd, int level, int optname,
                       void *optval, socklen_t *optlen);
extern getsockopt_fun getsockopt_f; 

typedef int (*setsockopt_fun)(int sockfd, int level, int optname,
                      const void *optval, socklen_t optlen);
extern setsockopt_fun setsockopt_f;

}

#endif