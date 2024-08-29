#ifndef __SYLAR_IOMANAGER_H__
#define __SYLAR_IOMANAGER_H__

#include "scheduler.h"
#include <memory>

namespace sylar{
class IOManager: public Scheduler{
public:
    typedef std::shared_ptr<IOManager> Ptr;
    typedef RWMutex RWMutexType;
    enum Event {
        NONE = 0x0,
        READ = 0x1,
        WRITE = 0x2,
    };

private:
    struct FdContext{
        typedef Mutex MutexType;
        struct EventContext {
            Scheduler* scheduler =nu; //事件执行的schduler
            Fiber::ptr fiber;    //事件协程
            std::function<void()>cb; //事件的回调函数
        };

        int fd;              //文件描述符(事件关联句柄)
        EventContext read;   //读事件
        EventContext write;  //写事件

        Event m_events =NONE; //已经注册的事件
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

private:
    int m_epfd=0;
    
}
#endif