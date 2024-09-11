#include "hook.h"
#include "sylar.h"

#include "log.h"
#include <iostream>
#include <dlfcn.h>
#include "fd_manager.h"
#include <functional>
/*并详细展示了如何通过钩子机制拦截系统调用（如 sleep 和 usleep），
并将这些调用挂接到一个基于协程（或称为“纤程”）的系统上*/
namespace sylar{

sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();    

static thread_local bool t_hook_enable =false;

#define HOOK_FUN(XX) \
   XX(sleep) \
   XX(usleep) \
   XX(nanosleep) \
   XX(socket) \
   XX(connect) \
   XX(accept)\
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

struct _HookIniter{
    _HookIniter(){
        hook_init();
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








extern "C"{
#define XX(name) name ## _fun name ## _f =nullptr;
       HOOK_FUN(XX);
#undef XX

struct timer_info{
    int canncelled =0;
};

template<typename OriginFun, typename ... Args>
static ssize_t do_io(int fd,OriginFun fun, const char* hook_fun_name,
    uint32_t event, int timeout_so, Args&&... args){
        if(!sylar::t_hook_enable){
            return fun(fd,std::forward<Args>(args)...)
        }

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
                    t->cancelled =ETIMEOUT;
                    iom->cancelEvent(fd,(sylar::IOManager::Event)(event));
                },winfo);
            }
            uint64_t now =0;

            int rt = iom->addEvent(fd,(sylar::IOManger::Event)(event));
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
/*这里为 sleep 和 usleep 函数指针声明了全局变量，并将其初始值设为 nullptr。
使用宏展开后，这部分等价于 sleep_fun sleep_f = nullptr;
usleep_fun usleep_f = nullptr;*/

unsigned int sleep(unsigned int seconds){
    if(!sylar::t_hook_enable){
        return sleep_f(seconds);
    }

    sylar::Fiber::ptr fiber =sylar::Fiber::GetThis();
    sylar::IOManager* iom=sylar::IOManager::GetThis();
    //iom->addTimer(seconds*1000,std::bind(&sylar::IOManager::schedule<sylar::Fiber::ptr>,iom,fiber));
    iom->addTimer(seconds*1000,[iom,fiber](){iom->schedule(fiber);});
    sylar::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec){
    if(!sylar::t_hook_enable){
        return usleep_f(usec);
    }

    sylar::Fiber::ptr fiber=sylar::Fiber::GetThis();
    sylar::IOManager* iom=sylar::IOManager::GetThis();
    //iom->addTimer(usec/1000,std::bind(&sylar::IOManager::schedule,iom,fiber));
    iom->addTimer(usec/1000,[iom,fiber](){iom->schedule(fiber);});
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
}


