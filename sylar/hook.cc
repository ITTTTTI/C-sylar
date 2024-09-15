#include "hook.h"
#include "sylar.h"

#include "log.h"
#include <iostream>
#include <dlfcn.h>
#include "fd_manager.h"
/*并详细展示了如何通过钩子机制拦截系统调用（如 sleep 和 usleep），
并将这些调用挂接到一个基于协程的系统上*/
sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");  
namespace sylar{

static sylar::ConfigVar<int>::ptr g_tcp_connect_timeout =
    sylar::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable =false;

#define HOOK_FUN(XX) \
   XX(sleep) \
   XX(usleep) \
   XX(nanosleep) \
   XX(socket) \
   XX(connect) \
   XX(accept) \
   XX(read)\
   XX(readv)\
   XX(recv)\
   XX(recvfrom)\
   XX(recvmsg)\
   XX(write)\
   XX(writev)\
   XX(send)\
   XX(sendto)\
   XX(sendmsg)\
   XX(close)\
   XX(fcntl)\
   XX(ioctl)\
   XX(getsockopt)\
   XX(setsockopt)

void hook_init(){
    static bool is_inited = false;
    if(is_inited)
    {
        return;
    }

#define XX(name) name ## _f = (name## _fun) dlsym(RTLD_NEXT,#name);
/*dlsym 动态获取函数的真实地址（如 sleep 和 usleep 的原始函数），
并将其存储到钩子函数指针 name_f 中.
RTLD_NEXT 告诉 dlsym 查找下一个库中的 name 函数地址（跳过当前库）*/
            HOOK_FUN(XX);
#undef XX
}

static uint64_t s_connct_timeout =-1;

struct _HookIniter{
    _HookIniter(){
        hook_init();
        s_connct_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addListener([](const int& old_value, const int& new_value){
        SYLAR_LOG_INFO(g_logger)<<"tcp connect timeout changed from "
        <<old_value<<" to "<<new_value;
        s_connct_timeout =new_value;
    });
    }
};


/*这里定义了一个 _HookIniter 结构体，其中的构造函数调用了 hook_init() 函数。
通过定义一个静态对象 s_hook_initer，
确保在 main 函数之前调用该初始化器，从而完成钩子的初始化工作。*/

static _HookIniter s_hook_initer;//为了实现在main函数之前调用执行一些函数

bool is_hook_enabled(){
    return t_hook_enable;
    
}
void set_hook_enabled(bool flag){
    t_hook_enable = flag;
}
}

struct timer_info{
    int cancelled =0;
  
};
  

template<typename OriginFun, typename ... Args>
//通过 hook 的方式来插入额外的逻辑（例如处理超时或非阻塞操作）
static ssize_t do_io(int fd,OriginFun fun, const char* hook_fun_name,
    uint32_t event, int timeout_so, Args&&... args){
        if(!sylar::t_hook_enable){
            return fun(fd,std::forward<Args>(args)...);
        }

        //SYLAR_LOG_DEBUG(g_logger)<<"do_io"<<hook_fun_name;

        sylar::FdCtx::ptr ctx=sylar::FdMgr::GetInstance()->get(fd);
        if(!ctx){
            return fun(fd,std::forward<Args>(args)...);
        }

        if(ctx->isClosed()){
            errno =EBADF;
            return -1;
        }

        if(!ctx->isSocket()|| ctx->getUserNonblock()){
            return fun(fd,std::forward<Args>(args)...);
        }

        uint64_t to = ctx->getTimeout(timeout_so);
        std::shared_ptr<timer_info> tinfo(new timer_info);
retry:
        ssize_t n = fun(fd,std::forward<Args>(args)...);
        while(n==-1 && errno ==EINTR){
            n= fun(fd,std::forward<Args>(args)...);
        }
        if(n==-1&&errno ==EAGAIN){
            SYLAR_LOG_DEBUG(g_logger)<<"do_io<"<<hook_fun_name<<">";
            sylar::IOManager* iom = sylar::IOManager::GetThis();
            sylar::Timer::ptr timer;
            std::weak_ptr<timer_info> winfo(tinfo);

            if(to!=(uint64_t)-1){
                timer =iom->addConditionTimer(to,[winfo,fd,iom,event](){
                    auto t=winfo.lock();
                    if(!t||t->cancelled)
                    {
                        return;
                    }
                    t->cancelled =ETIMEDOUT;
                    iom->cancelEvent(fd,(sylar::IOManager::Event)(event));
                },winfo);
            }

            int rt = iom->addEvent(fd,(sylar::IOManager::Event)(event));
            if(rt){

                    SYLAR_LOG_ERROR(g_logger)<<hook_fun_name<<"addEvent(" 
                    <<fd <<','<<event<<")";
                if(timer){
                    timer->cancel();
                }
                return -1;
            }else {
                sylar::Fiber::YieldToHold();
                if(timer){
                    timer->cancel();
                }
                if(tinfo->cancelled){
                    errno=tinfo->cancelled;
                    return -1;
                }

                goto retry;
            }

        }
        return n;
}


extern "C"{
#define XX(name) name ## _fun name ## _f =nullptr;
       HOOK_FUN(XX);
#undef XX

/*这里为 sleep 和 usleep 函数指针声明了全局变量，并将其初始值设为 nullptr。
使用宏展开后，这部分等价于 sleep_fun sleep_f = nullptr;
usleep_fun usleep_f = nullptr;*/



unsigned int sleep(unsigned int seconds){
    if(!sylar::t_hook_enable){
        return sleep_f(seconds);
    }

    sylar::Fiber::ptr fiber =sylar::Fiber::GetThis();
    sylar::IOManager* iom=sylar::IOManager::GetThis();
    iom->addTimer(seconds*1000,
    std::bind((void(sylar::Scheduler::*)(sylar::Fiber::ptr,int thread))&sylar::IOManager::schedule,iom,fiber,-1));
    
    sylar::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec){
    if(!sylar::t_hook_enable){
        return usleep_f(usec);
    }

    sylar::Fiber::ptr fiber=sylar::Fiber::GetThis();
    sylar::IOManager* iom=sylar::IOManager::GetThis();
    iom->addTimer(usec/1000,
    std::bind((void(sylar::Scheduler::*)(sylar::Fiber::ptr,int thread))&sylar::IOManager::schedule,iom,fiber,-1));
    sylar::Fiber::YieldToHold();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem){
    if(!sylar::t_hook_enable){
        return nanosleep_f(req,rem);
    }

    int timeout_ms = req->tv_sec *1000+req->tv_nsec/1000/1000;
    sylar::Fiber::ptr fiber=sylar::Fiber::GetThis();
    sylar::IOManager* iom=sylar::IOManager::GetThis();
    //iom->addTimer(usec/1000,std::bind(&sylar::IOManager::schedule,iom,fiber));
    iom->addTimer(timeout_ms,[iom,fiber](){iom->schedule(fiber);});
    sylar::Fiber::YieldToHold();
    return 0;
}


int socket(int domain, int type, int protocol){
    if(!sylar::t_hook_enable){
        return socket_f(domain,type,protocol);
    }
    int fd =socket_f(domain,type,protocol);
    if(fd ==-1){
        return fd;
    }

    sylar::FdMgr::GetInstance()->get(fd,true);
    return fd;
}

int connect_with_timeout(int fd , const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms)
{
    SYLAR_LOG_INFO(g_logger)<<"connect_with_timeout"<<fd<<" "<<timeout_ms;
    if(!sylar::t_hook_enable){
        return connect_f(fd,addr,addrlen);
    }
    sylar::FdCtx::ptr ctx =sylar::FdMgr::GetInstance()->get(fd);
    if(!ctx||ctx->isClosed()||!ctx->isSocket())
    {
        return connect_f(fd,addr,addrlen);
    }

    if(ctx->getUserNonblock())
    {
        return connect_f(fd,addr,addrlen);
    }
    int n=connect_f(fd,addr,addrlen);
    if(n==0){
        return 0;
    }else if(n!=-1||errno!=EINPROGRESS){
        return n;
    }
    sylar::IOManager* iom = sylar::IOManager::GetThis();
    sylar::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    if(timeout_ms != (uint64_t)-1){
            timer= iom->addConditionTimer(timeout_ms,[winfo,fd,iom](){
            auto t=winfo.lock();
            if (!t||t->cancelled){
                return;
            }
            t->cancelled =ETIMEDOUT;
            iom->cancelEvent(fd,sylar::IOManager::WRITE);     
        },winfo);
    }

    int rt =iom->addEvent(fd,sylar::IOManager::WRITE);
    if(rt==0){
        sylar::Fiber::YieldToHold();
        if(timer){
            timer->cancel();
        }
        if(tinfo->cancelled){
            errno=tinfo->cancelled;
            return -1;
        }
        }else{
            if(timer){
                timer->cancel();
            }
            SYLAR_LOG_ERROR(g_logger)<<"connect addEvent ("<<fd<<", WRITE) error";

        }
        int error =0;
        socklen_t len =sizeof(int);
        if(-1==getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)){
            return -1;
        }
        if(!error){
            return 0;
        }else{
            errno=error;
            return -1;
        }

}


int connect(int sockfd, const struct sockaddr *addr,socklen_t addrlen){
    return connect_with_timeout(sockfd,addr,addrlen,sylar::s_connct_timeout);
}


int accept(int s, struct sockaddr *addr,socklen_t *addrlen){
    int fd = do_io(s, accept_f,"accept",sylar::IOManager::READ,SO_RCVTIMEO, addr, addrlen);
    if(fd>=0){
        sylar::FdMgr::GetInstance()->get(fd,true);
    }
    return fd;
}

ssize_t read(int fd, void *buf, size_t count){
    return do_io(fd,read_f,"read",sylar::IOManager::READ, SO_RCVTIMEO, buf,count);
}


ssize_t readv(int fd, const struct iovec *iov, int iovcnt){
    return do_io(fd,readv_f,"readv",sylar::IOManager::READ,SO_RCVTIMEO, iov,iovcnt);
}


ssize_t recv(int sockfd, void *buf, size_t len, int flags){
    return do_io(sockfd,recv_f,"recv",sylar::IOManager::READ,SO_RCVTIMEO,buf,len,flags);
}


ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen){
        return do_io(sockfd,recvfrom_f,"recvfrom",sylar::IOManager::READ,SO_RCVTIMEO,buf,len,flags,src_addr,addrlen);
                            
}



ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags){
    return do_io(sockfd,recvmsg_f,"recvmsg",sylar::IOManager::READ,SO_RCVTIMEO,msg,flags);
    
}

ssize_t write(int fd, const void *buf, size_t count){
    return do_io(fd,write_f,"write",sylar::IOManager::WRITE,SO_SNDTIMEO,buf,count);
}


ssize_t writev(int fd, const struct iovec *iov, int iovcnt){
    return do_io(fd,writev_f,"writev",sylar::IOManager::WRITE,SO_SNDTIMEO,iov,iovcnt);
}


ssize_t send(int sockfd, const void *buf, size_t len, int flags){
    return do_io(sockfd,send_f,"send",sylar::IOManager::WRITE,SO_SNDTIMEO,buf,len,flags);
}

ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                        const struct sockaddr *dest_addr, socklen_t addrlen){
    return do_io(sockfd,sendto_f,"sendto",sylar::IOManager::WRITE,SO_SNDTIMEO,buf,len,flags,dest_addr,addrlen);
                            
}



ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags){
    return do_io(sockfd,sendmsg_f,"sendmsg",sylar::IOManager::WRITE,SO_SNDTIMEO,msg,flags);
}

int close(int fd){
    if(!sylar::t_hook_enable){
        return close_f(fd);
    }

    sylar::FdCtx::ptr ctx = sylar::FdMgr::GetInstance()->get(fd);
    if(ctx){
        auto iom=sylar::IOManager::GetThis();
        if(iom){
            iom->cancelAll(fd);
        } 
        sylar::FdMgr::GetInstance()->del(fd);      
    }
    return close_f(fd);
}

int fcntl(int fd, int cmd,.../*arg*/){
    va_list va;//va_list是一个类型，用于在函数中遍历可变参数列表。
    va_start(va,cmd);
    //这行代码初始化了va_list变量va，使其指向可变参数列表的第一个参数。这是通过va_start宏完成的，它需要两个参数
    switch(cmd){
        case F_SETFL:
        {
            int arg =va_arg(va,int);
            va_end(va);
            sylar::FdCtx::ptr ctx=sylar::FdMgr::GetInstance()->get(fd);
            if(!ctx||ctx->isClosed()||!ctx->isSocket()){
                return fcntl_f(fd,cmd,arg);
            }
            ctx->setUserNonblock(arg& O_NONBLOCK);
            if(ctx->getsysNonblock()){
                arg |= O_NONBLOCK;
            }else{
                arg&=~O_NONBLOCK;
            }
            return fcntl_f(fd,cmd,arg);
        }
        break;
        case F_GETFL:
        {
            va_end(va);
            int arg=fcntl_f(fd,cmd);
            sylar::FdCtx::ptr ctx=sylar::FdMgr::GetInstance()->get(fd);
            if(!ctx||ctx->isClosed()||!ctx->isSocket()){
                return arg;
            }
            if(ctx->getUserNonblock())
            {
                return arg| O_NONBLOCK;
            }else{
                return arg &~O_NONBLOCK;
            }

        }
        break;
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_ADD_SEALS
        case F_SETPIPE_SZ:
#endif
        {
            int arg =va_arg(va,int);
            va_end(va);
            return fcntl_f(fd,cmd,arg);
        }
            break;
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
        case F_GETPIPE_SZ:
        {
            va_end(va);
            return fcntl_f(fd,cmd);
        }
            break;

        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
        {
              struct flock* arg = va_arg(va,struct flock*);
              va_end(va);
              return fcntl_f(fd,cmd,arg);
        }
            break;

        case F_SETOWN_EX:
        case F_GETOWN_EX:
        {
            struct f_owner_exlock* arg =va_arg(va,struct f_owner_exlock*);
            va_end(va);
            return fcntl_f(fd,cmd,arg);
        }
            break;
        default:
            va_end(va);
            return fcntl_f(fd,cmd);
        }
}


 int ioctl(int fd, unsigned long request, ...){
    va_list va;
    va_start(va,request);
    void* arg = va_arg(va,void*);
    va_end(va);

    if(FIONBIO==request){
        bool user_nonblock = !!*(int*)arg;
        sylar::FdCtx::ptr ctx=sylar::FdMgr::GetInstance()->get(fd);
        if(!ctx||ctx->isClosed()||!ctx->isSocket())
        {
            return ioctl_f(fd,request,arg);
        }
        ctx->setUserNonblock(user_nonblock);
    }
    return ioctl_f(fd,request,arg);
 }

 int getsockopt(int sockfd, int level, int optname,
                void *optval, socklen_t *optlen){
                    return getsockopt_f(sockfd,level,optname,optval,optlen);    
                }

 int setsockopt(int sockfd, int level, int optname,
               const void *optval, socklen_t optlen){
        if(!sylar::t_hook_enable){
            return setsockopt_f(sockfd,level,optname,optval,optlen);
        }
        if(level==SOL_SOCKET){
            if(optname==SO_RCVTIMEO || optname==SO_SNDTIMEO)
                //操作是否是设置接收超时时间和发送超时时间
            {
                sylar::FdCtx::ptr ctx=sylar::FdMgr::GetInstance()->get(sockfd);
                if(ctx){
                    const timeval* tv=(const timeval*)optval;
                    ctx->setTimeout(optname,tv->tv_sec*100+tv->tv_usec/1000);

                }
            }
        }
        return setsockopt_f(sockfd,level,optname,optval,optlen);
}



}


