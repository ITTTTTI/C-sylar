#include "fiber.h"
#include "config.h"
#include "macro.h"
#include "log.h"
#include <functional>
#include <iostream>
#include <atomic>
#include <memory>

namespace sylar{

static Logger::ptr g_logger=SYLAR_LOG_NAME("system");
static std::atomic<uint64_t> s_fiber_id {0};
/*std::atomic<T> 是C++11及以后版本中引入的一个模板类，用于提供原子操作.
原子操作是指在执行过程中不会被线程调度机制中断的操作，
这种操作在多线程编程中非常重要，因为它可以确保数据的一致性和线程安全。*/
static std::atomic<uint64_t> s_fiber_count {0};

static thread_local Fiber* t_fiber =nullptr;//当前协程
//这行代码定义了一个线程局部的指针t_fiber，它指向Fiber类型的对象，并且每个线程都有自己的t_fiber副本，初始值为nullptr
static thread_local Fiber::ptr t_threadFiber =nullptr;//主协程

static ConfigVar<uint32_t>::ptr g_fiber_stack_size= Config::Lookup<uint32_t>("fiber.stack_size",1024*1024*1,"fiber stack size");

class MallocStackAllocator{
public:
    static void* Alloc(size_t size){
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size){
        return free(vp);
    }

};    

using StackAllocator=MallocStackAllocator; //使用了using关键字来定义一个新的类型名StackAllocator，这个新类型名实际上是现有类型MallocStackAllocator的一个别名

uint64_t Fiber::GetFiberId(){
    if(t_fiber){
        return t_fiber->getId();
    }
    return 0;
}
//主协程
Fiber::Fiber(){
    m_state=EXEC;
    SetThis(this);//this指针在构造函数中传递对象自身的引用或指针是一种常见的做法，特别是当类需要管理或引用其他对象或资源时。

    if(getcontext(&m_ctx)){//getcontext 函数在成功时返回0，在失败时返回-1
        SYLAR_ASSERT2(false,"getcontext");
    }

    ++s_fiber_count;
}
// uint64_t m_id=0;//协程id
// uint32_t m_stacksize =0;//协程栈大小
// State m_state = INIT; //协程状态

// ucontext_t m_ctx; //协程上下文
// void* m_stack =nullptr; //协程栈

// std::function<void()> m_cb; //协程函数
Fiber::Fiber(std::function<void()> cb,size_t stacksize):m_id(++s_fiber_id),m_cb(cb){
    ++s_fiber_count;
    m_stacksize=stacksize?stacksize:g_fiber_stack_size->getValue();
    m_stack=StackAllocator::Alloc(m_stacksize);
    if(getcontext(&m_ctx)){
        SYLAR_ASSERT2(false,"getcontext");
    }

    m_ctx.uc_link=nullptr;
    /*这行代码将m_ctx结构体中的uc_link成员设置为nullptr。
    ucontext_t结构体中的uc_link成员是一个指向另一个ucontext_t的指针，
    用于在通过setcontext或setjmp等函数恢复上下文时，指定一个“链接”的上下文。
    如果uc_link非空，则当当前上下文执行完成后，会自动跳转到uc_link指向的上下文继续执行*/
    m_ctx.uc_stack.ss_sp=m_stack; //栈指针（ss_sp）设置为m_stack。m_stack是一个指向内存区域的指针，这个内存区域被用作这个上下文的栈空间
    m_ctx.uc_stack.ss_size=m_stacksize;//这行代码设置这个栈空间的大小为m_stacksize。ss_size是栈的大小，以字节为单位。

    makecontext(&m_ctx, &Fiber::MainFunc, 0);//这行代码使用makecontext函数来初始化m_ctx上下文，以便它可以执行Fiber类的MainFunc成员函数。makecontext的第一个参数是指向ucontext_t结构的指针，即我们要设置的上下文。

}
Fiber::~Fiber(){
    --s_fiber_count;
    if(m_stack){
        SYLAR_ASSERT(m_state==TERM||m_state==INIT||m_state==EXCEPT);
        StackAllocator::Dealloc(m_stack, m_stacksize);
    }
    else{
        SYLAR_ASSERT(!m_cb);
        SYLAR_ASSERT(m_state==EXEC);

        Fiber* cur = t_fiber;
        if(cur ==this){
            SetThis(nullptr);
        }
    }
    
}
void Fiber::reset(std::function<void()>cb){
    SYLAR_ASSERT(m_stack);
    SYLAR_ASSERT(m_state==TERM||m_state==INIT||m_state==EXCEPT);
    m_cb=cb;
    if(getcontext(&m_ctx)){
    SYLAR_ASSERT2(false,"getcontext");
    }

    m_ctx.uc_link=nullptr;
    m_ctx.uc_stack.ss_sp=m_stack; 
    m_ctx.uc_stack.ss_size=m_stacksize;

    makecontext(&m_ctx, &Fiber::MainFunc, 0);
    m_state=INIT;

}
//切换到当前协程执行
void Fiber::swapIn(){
    SetThis(this);
    SYLAR_ASSERT(m_state!=EXEC);
    m_state=EXEC;

    if(swapcontext(&t_threadFiber->m_ctx,&m_ctx)){
        SYLAR_ASSERT2(false,"swapcontext");
    }
    
}
//切换到后台执行
void Fiber::swapOut(){
    SetThis(t_threadFiber.get());
    if(swapcontext(&m_ctx,&t_threadFiber->m_ctx)){
        SYLAR_ASSERT2(false,"swapcontext");
    }

}

void Fiber::SetThis(Fiber* f){
    t_fiber=f;
}

Fiber::ptr Fiber::GetThis(){
    if(t_fiber){
        return t_fiber->shared_from_this();//类的成员函数内部安全地生成一个指向该类当前实例的 std::shared_ptr。
    }
    Fiber::ptr main_fiber(new Fiber);
    SYLAR_ASSERT(t_fiber==main_fiber.get());
    t_threadFiber=main_fiber;
    return t_fiber->shared_from_this();

}
//协程切换到后台，状态ready
void Fiber::YieldToReady(){
    Fiber::ptr cur=GetThis();
    cur->m_state=READY;
    cur->swapOut();
}
//协程切换到后台，状态hold
void Fiber::YieldToHold(){
    Fiber::ptr cur=GetThis();
    cur->m_state=HOLD;
    cur->swapOut();
}
//总协程数
uint64_t TotalFibers(){
    return s_fiber_count;
}

void Fiber::MainFunc(){
    Fiber::ptr cur=GetThis();
    SYLAR_ASSERT(cur);
    try
    {
        cur->m_cb();
        cur->m_cb=nullptr;
        cur->m_state=TERM;

    }
    catch(std::exception& e){
        cur->m_state=EXCEPT;
        SYLAR_LOG_ERROR(g_logger)<<"Fiber Except: "<<e.what();
    }
    catch(...){
        SYLAR_LOG_ERROR(g_logger)<<"Fiber Except: ";

    }

    auto raw_ptr=cur.get();
    cur.reset();
    raw_ptr->swapOut();

}

}