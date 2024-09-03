#include "iomanager.h"
#include "macro.h"
#include "log.h"

#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>


namespace sylar
{
static sylar::Logger::ptr g_logger=SYLAR_LOG_NAME("system");

IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event){
        switch(event){
            case IOManager::READ:
                 return read;
            case IOManager::WRITE:
                 return write;
            default:
                 SYLAR_ASSERT2(false,"getContext");
        }
}              
void IOManager::FdContext::resetContext(EventContext& ctx){
    ctx.scheduler =nullptr;
    ctx.fiber.reset();
    ctx.cb=nullptr;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event){
       SYLAR_ASSERT(events &event);
       events=(Event)(events & ~event);
       EventContext& ctx =getContext(event);
       if(ctx.cb){
        ctx.scheduler->schedule(&ctx.cb);
       }else{
         ctx.scheduler->schedule(&ctx.fiber);
       }
       ctx.scheduler=nullptr;
       return;
}


IOManager::IOManager(size_t threads, bool use_caller, const std::string& name)
:Scheduler(threads,use_caller,name){
    m_epfd=epoll_create(512);
    SYLAR_ASSERT(m_epfd>0);

    int rt =pipe(m_tickleFds);//m_tickleFds[0] 是读取端，m_tickleFds[1] 是写入端
    //允许一个进程（称为写进程）将数据写入管道的一端，而另一个进程（称为读进程）可以从管道的另一端读取数据。
    //整型数组，用于存储管道两端的文件描述符。pipefd[0] 是管道的读端，而 pipefd[1] 是管道的写端。
    SYLAR_ASSERT(!rt);

    epoll_event event;//结构体用于描述 epoll 要监听的事件
    /*typedef union epoll_data {
    void        *ptr;
    int          fd;
    __uint32_t   u32;
    __uint64_t   u64;
} epoll_data_t;

   struct epoll_event {
    __uint32_t   events; //events：这是一个位掩码，用于指定事件类型。最常见的事件类型包括
EPOLLIN：表示对应的文件描述符上有数据可读。
EPOLLOUT：表示对应的文件描述符是写就绪的。
EPOLLERR：表示对应的文件描述符发生错误。
EPOLLHUP：表示对应的文件描述符被挂起（比如，对端关闭了连接）。
EPOLLET：将事件设置为边缘触发（Edge Triggered）模式，而不是默认的水平触发（Level Triggered）模式。   
    epoll_data_t data;     
    data：这是一个联合体（union），用于存储与事件关联的数据。根据使用场景的不同，可以选择不同的字段来存储数据： 
}*/
    memset(&event,0,sizeof(epoll_event)); //将 event 结构体的内存清零，以确保没有残留数据。
    event.events=EPOLLIN|EPOLLET; //设置监听的事件类型。EPOLLIN 表示监听可读事件，EPOLLET 表示使用边缘触发模式（edge-triggered）
    event.data.fd=m_tickleFds[0]; //将 m_tickleFds[0]（管道的读取端）赋值给 event.data.fd，以便 epoll 可以监控该文件描述符的事件

    rt =fcntl(m_tickleFds[0], F_SETFL,O_NONBLOCK); //fcntl 将 m_tickleFds[0] 设置为非阻塞模式。这样在读取管道时，如果没有数据，程序不会阻塞。
    SYLAR_ASSERT(!rt);

    rt=epoll_ctl(m_epfd,EPOLL_CTL_ADD,m_tickleFds[0],&event); // m_tickleFds[0] 和 event 添加到 epoll 实例中进行监控。EPOLL_CTL_ADD 表示添加新的文件描述符到 epoll
    SYLAR_ASSERT(rt);

    contextResize(32);

    start();

}

IOManager::~IOManager(){
    stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for(size_t i=0; i< m_FdContexts.size();++i)
    {
        if(m_FdContexts[i])
        {
            delete m_FdContexts[i];
        }
    }
    


}

void IOManager::contextResize(size_t size){
    m_FdContexts.resize(size);

    for(size_t i=0;i<size;++i){
        if(!m_FdContexts[i]){
            m_FdContexts[i]=new FdContext;
            m_FdContexts[i]->fd=i;
        }
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb){
    FdContext* fd_ctx =nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_FdContexts.size()>fd){
        fd_ctx =m_FdContexts[fd]; 
        lock.unlock();  
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd*1.5);
        fd_ctx =m_FdContexts[fd];
    }
    FdContext::MutexType::Lock lock2(fd_ctx->mutex);

    if(fd_ctx->events &  event){
        SYLAR_LOG_ERROR(g_logger)<<"addEvent assert fd="<<fd<<"event="<<event
                                <<"fd_ctx.event"<<fd_ctx->events;
        SYLAR_ASSERT(!(fd_ctx->events & event));
        //SYLAR_LOG_ERROR(g_logger)<<""
    }

    int op= fd_ctx->events ? EPOLL_CTL_MOD :EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events= EPOLLET|fd_ctx->events|event;
    epevent.data.ptr=fd_ctx;

    int rt=epoll_ctl(m_epfd,op,fd,&epevent);
    if(rt){
        SYLAR_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd <<","
                        <<op<<","<<fd<<","<<epevent.events<<"):"
                        <<rt<<" ("<<errno<<") ("<<strerror(errno)<<")";
        return -1;
    }

    ++m_pendingEventCount;
    fd_ctx->events=(Event)(fd_ctx->events|event);
    FdContext::EventContext& event_ctx =fd_ctx->getContext(event);
    SYLAR_ASSERT(!event_ctx.scheduler&&!event_ctx.fiber&&!event_ctx.cb);
    event_ctx.scheduler=Scheduler::GetThis();
    if(cb)
    {
        event_ctx.cb.swap(cb);
    }
    else{
        event_ctx.fiber=Fiber::GetThis();
        SYLAR_ASSERT(event_ctx.fiber->getState()==Fiber::EXEC);
        
    }
    return 0;

}
bool IOManager::delEvent(int fd,Event event){
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_FdContexts.size()<=fd){
        return false;
    }
    FdContext* fd_ctx =m_FdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events & event)){
        return false;
    }

    Event new_events= (Event)(fd_ctx->events & ~event);
    int op= fd_ctx->events ? EPOLL_CTL_MOD :EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET| new_events;
    epevent.data.ptr = fd_ctx;

    int rt =epoll_ctl(m_epfd,op,fd,&epevent);
    if(rt){
          SYLAR_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd <<","
                        <<op<<","<<fd<<","<<epevent.events<<"):"
                        <<rt<<" ("<<errno<<") ("<<strerror(errno)<<")";
            return false;
    }

    --m_pendingEventCount;
    fd_ctx->events=new_events;
    FdContext::EventContext& event_ctx =fd_ctx->getContext(event);
    fd_ctx->resetContext(event_ctx);
    return true;
}
bool IOManager::cancelEvent(int fd, Event event){
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_FdContexts.size()<=fd){
        return false;
    }
    FdContext* fd_ctx =m_FdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!(fd_ctx->events & event)){
        return false;
    }

    Event new_events= (Event)(fd_ctx->events & ~event);
    int op= fd_ctx->events ? EPOLL_CTL_MOD :EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET| new_events;
    epevent.data.ptr = fd_ctx;

    int rt =epoll_ctl(m_epfd,op,fd,&epevent);
    if(rt){
          SYLAR_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd <<","
                        <<op<<","<<fd<<","<<epevent.events<<"):"
                        <<rt<<" ("<<errno<<") ("<<strerror(errno)<<")";
            return false;
    }

    fd_ctx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancelAll(int fd){
        RWMutexType::ReadLock lock(m_mutex);
    if((int)m_FdContexts.size()<=fd){
        return false;
    }
    FdContext* fd_ctx =m_FdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!fd_ctx->events){
        return false;
    }


    int op= EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt =epoll_ctl(m_epfd,op,fd,&epevent);
    if(rt){
          SYLAR_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd <<","
                        <<op<<","<<fd<<","<<epevent.events<<"):"
                        <<rt<<" ("<<errno<<") ("<<strerror(errno)<<")";
            return false;
    }
    if(fd_ctx->events & READ){
        fd_ctx->triggerEvent(READ);
        --m_pendingEventCount;
    }

    if(fd_ctx->events & WRITE){
        fd_ctx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }
    
    SYLAR_ASSERT(fd_ctx->events==0);
    return true;
    
}

IOManager* IOManager::GetThis(){
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::tickle() {
    if(!hasIdleThreads())
    {
        return;
    }
    int rt =write(m_tickleFds[1],"T",1);
    SYLAR_ASSERT(rt==1);
}
bool IOManager::stopping() {
    return Scheduler::stopping()
            && m_pendingEventCount==0;


};
void IOManager::idle() {
    epoll_event* events = new epoll_event[64];
    std::shared_ptr<epoll_event> shared_envents(events,[](epoll_event* ptr){delete[] ptr;});
    while(true)
    {
        if(stopping()){
            SYLAR_LOG_INFO(g_logger)<<"name="<<getName()<<" idle";
        }
        int rt =0;
        do {
            static const int MAX_TIMEOUT =5000;//这个常量用作epoll_wait函数的超时时间
            rt =epoll_wait(m_epfd,events,64,MAX_TIMEOUT);
            /*这是一个指向epoll_event结构数组的指针，该数组用于存储epoll_wait函数检测到的事件。
            这是events数组的大小，即最多可以处理的事件数量。这意味着如果有超过64个事件同时就绪,
            那么只有前64个事件会被处理，其余的事件将等待下一次调用epoll_wait时处理。*/

            if(rt<0 && errno ==EINTR){

            }else{
                break;
            }
        }while(true);

        for(int i=0; i< rt ;i++)
        {
            epoll_event& event =events[i];
            if(event.data.fd==m_tickleFds[0]){
                uint8_t dummy;
                while(read(m_tickleFds[0],&dummy,1)==1);
                continue;
            }

            FdContext* fd_ctx= (FdContext*)event.data.ptr;
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            if(event.events & (EPOLLERR|EPOLLHUP)){
                event.events |=EPOLLIN|EPOLLOUT;
            }
            int real_events =NONE;
            if(event.events & EPOLLIN){
                real_events |= READ;
            }
            if(event.events & EPOLLOUT ){
                real_events |= WRITE;
            }


            if(fd_ctx->events & real_events){
                continue;
            }

            int left_events = (fd_ctx->events & ~real_events);
            int op =left_events ?EPOLL_CTL_MOD:EPOLL_CTL_DEL;
            event.events = EPOLLET| left_events;

            int rt2=epoll_ctl(m_epfd,op,fd_ctx->fd,&event);
            if(rt2){
                SYLAR_LOG_ERROR(g_logger)<<"epoll_ctl("<<m_epfd <<","
                        <<op<<","<<fd_ctx->fd<<","<<event.events<<"):"
                        <<rt2<<" ("<<errno<<") ("<<strerror(errno)<<")";
                continue;
            }

            if(real_events & READ){
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_events & WRITE){
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }
    }
    Fiber::ptr cur =Fiber::GetThis();
    auto raw_ptr =cur.get();
    cur.reset();

    raw_ptr->swapOut();
    
}

} // namespace name
