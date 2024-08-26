#ifndef __SYLAR_FIBER_H__
#define __SYLAR_FIBER_H__

#include <memory>//智能指针
#include <functional>//函数对象
#include <ucontext.h>
#include "thread.h"
namespace sylar
{

class Scheduler;
class Fiber : public std::enable_shared_from_this<Fiber>{
friend class Scheduler;
public:
    typedef std::shared_ptr<Fiber> ptr;
    enum State{
        INIT, //初始化状态
        HOLD, //处于等待或暂停状态
        EXEC, //Fiber 对象正在执行
        TERM, //对象已终止或结束执行
        READY, 
        EXCEPT, //Fiber 对象已准备好执行，但尚未开始
    };
private:
    Fiber();//禁用默认构造函数
public:
    Fiber(std::function<void()> cb,size_t stacksize =0);
    ~Fiber();
    //重置协程函数，并重置状态
    void reset(std::function<void()>cb);
    //切换到当前协程执行
    void swapIn();
    //切换到后台执行
    void swapOut();

    uint64_t getId() const {return m_id;}
    State getState() const {return m_state;}
public:
    //设置当前协程对象
    static void SetThis(Fiber* f);
    //获取当前协程对象指针
    static Fiber::ptr GetThis();
    //协程切换到后台，状态ready
    static void YieldToReady();
    //协程切换到后台，状态hold
    static void YieldToHold();
    //总协程数
    static uint64_t TotalFibers();

    static void MainFunc();
    static uint64_t GetFiberId();

private:
    uint32_t m_stacksize =0;//协程栈大小
    State m_state = INIT; //协程状态
    uint64_t m_id=0; //协程id

    ucontext_t m_ctx; //协程上下文
    void* m_stack =nullptr; //协程栈

    std::function<void()> m_cb; //协程函数

};
} 


#endif