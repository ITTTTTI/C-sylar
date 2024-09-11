#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__

#include "scheduler.h"
#include "timer.h"
#include <memory>

namespace sylar{
class IOManager: public Scheduler , public TimerManager{
public:
    typedef std::shared_ptr<IOManager> Ptr;
    typedef RWMutex RWMutexType;
    enum Event {
        NONE = 0x0,
        READ = 0x1,
        WRITE = 0x4,
    };

private:
    struct FdContext{
        typedef Mutex MutexType;
        struct EventContext {
            Scheduler* scheduler =nullptr; //事件执行的schduler
            Fiber::ptr fiber;    //事件协程
            std::function<void()>cb; //事件的回调函数
        };

        int fd =0;
        EventContext& getContext(Event event);              
        void resetContext(EventContext& ctx);     
        void triggerEvent(Event event); //触发事件
        EventContext read;   //读事件
        EventContext write;  //写事件

        Event events =NONE; //已经注册的事件
        MutexType mutex;
    };
public:
    IOManager(size_t threads =1, bool use_caller =true, const std::string& name="");
    ~IOManager();

    int addEvent(int fd, Event event, std::function<void()> cb=nullptr);
    bool delEvent(int fd,Event event);
    bool cancelEvent(int fd, Event event);

    bool cancelAll(int fd);

    static IOManager* GetThis();

protected:
    void tickle() override;
    bool stopping() override;
    
    void idle() override;
    void onTimerInsertedAtfront() override;

    void contextResize(size_t size);
    bool stopping(uint64_t& timeout);

private:
    int m_epfd=0; //epoll的文件描述符
    int m_tickleFds[2]; //用于epoll_wait的管道文件描述符
    
    std::atomic<size_t> m_pendingEventCount={0}; //等待的事件数量
    RWMutexType m_mutex;  //用于保护m_FdContexts
    std::vector<FdContext*> m_FdContexts;  //文件描述符对应的上下文
    
};
}
#endif