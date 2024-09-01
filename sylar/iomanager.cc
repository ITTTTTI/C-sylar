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
sylar::Logger::ptr g_logger=SYLAR_LOG_ROOT();
IOManager::IOManager(size_t threads =1, bool use_caller =true, const std::string& name="")
:Scheduler(threads,use_caller,name){
    m_epfd=epoll_create(512);
    SYLAR_ASSERT(m_epfd>0);

    int rt =pipe(m_tickleFds);//m_tickleFds[0] 是读取端，m_tickleFds[1] 是写入端
    SYLAR_ASSERT(rt);

    epoll_event event;
    memset(&event,0,sizeof(epoll_event)); //将 event 结构体的内存清零，以确保没有残留数据。
    event.events=EPOLLIN|EPOLLET; //设置监听的事件类型。EPOLLIN 表示监听可读事件，EPOLLET 表示使用边缘触发模式（edge-triggered）
    event.data.fd=m_tickleFds[0]; //将 m_tickleFds[0]（管道的读取端）赋值给 event.data.fd，以便 epoll 可以监控该文件描述符的事件

    rt =fcntl(m_tickleFds[0], F_SETFL,O_NONBLOCK); //fcntl 将 m_tickleFds[0] 设置为非阻塞模式。这样在读取管道时，如果没有数据，程序不会阻塞。
    SYLAR_ASSERT(rt);

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

int IOManager::addEvent(int fd, Event event, std::function<void()> cb=nullptr){
    FdContext* fd_ctx =nullptr;
    RWMutexType::ReadLock lock(m_mutex);
    if(m_FdContexts.size()>fd){
        fd_ctx =m_FdContexts[fd]; 
        lock.unlock();  
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(m_FdContexts.size()*1.5);
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
    if(m_FdContexts.size()<=fd){
        return false;
    }
    FdContext* fd_ctx =m_FdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock(fd_ctx->mutex);
}
bool IOManager::cancelEvent(int fd, Event event){
    
}

bool IOManager::cancelAll(int fd){
    
}

IOManager* IOManager::GetThis(){
    
}

void IOManager::tickle() {}
bool IOManager::stopping() {

};
void IOManager::idle() {
    
}

} // namespace name
