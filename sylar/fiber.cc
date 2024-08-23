#include "fiber.h"
#include <functional>
#include <iostream>
#include <atomic>
#include <memory>

namespace sylar{
static std::atomic<uint64_t> s_fiber_id {0};
/*std::atomic<T> 是C++11及以后版本中引入的一个模板类，用于提供原子操作.
原子操作是指在执行过程中不会被线程调度机制中断的操作，
这种操作在多线程编程中非常重要，因为它可以确保数据的一致性和线程安全。*/
static std::atomic<uint64_t> s_fiber_count {0};

class MallocStackAllocator{
public:
    static void* Alloc(size_t size){
        return malloc(size);
    }

    static void Dealloc(void* vp, size_t size){
        return free(vp);
    }

};    
Fiber::Fiber(std::function<void()> cb,size_t stacksize =0){

}
Fiber::~Fiber(){
    
}
void Fiber::reset(std::function<void()>cb){

}
//切换到当前协程执行
void Fiber::swapIn(){
    
}
//切换到后台执行
void Fiber::swapOut(){

}
static Fiber::ptr GetThis(){
    
}
//协程切换到后台，状态ready
static void YieldToReady(){
    
}
//协程切换到后台，状态hold
static void YieldToHold(){
    
}
//总协程数
static uint64_t TotalFibers(){
    
}

static int MainFunc(){
    
}

}