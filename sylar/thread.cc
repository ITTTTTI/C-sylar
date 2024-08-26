#include "thread.h"
#include <string>
#include <functional>
#include <iostream>
#include "log.h"
#include "util.h"


namespace sylar{

static thread_local Thread* t_thread =nullptr;
static thread_local std::string t_thread_name= "UNKNOW";
//thread_local：它声明的变量对于每个线程来说都是独立的，也就是说每个线程都有自己的一份独立的 t_thread 和 t_thread_name。不同线程之间对这些变量的修改互不影响。
static sylar::Logger::ptr g_logger =SYLAR_LOG_NAME("system");

Semaphore::Semaphore(uint32_t count){
    // 初始化信号量
    if(sem_init(&m_semaphore,0,count)){
        /*sem_init(&m_semaphore, 0, count)：这个函数用于初始化由m_semaphore指向的信号量对象。
         第一个参数&m_semaphore是指向信号量对象的指针。
         第二个参数0（或PTHREAD_PROCESS_SHARED的宏定义，如果使用了共享内存或进程间同步）指定了信号量的作用域。在这里，使用0表示信号量仅在进程内部共享（即，它是线程间的同步机制）。
         第三个参数count是信号量的初始值。这个值必须是非负的。当信号量的值大于0时，表示有资源可用；当信号量的值为0时，表示没有资源可用，任何尝试获取资源的线程都将被阻塞，直到信号量的值被其他线程或进程增加。*/
        throw std::logic_error("sem_init error");
    }

}

Semaphore::~Semaphore(){
    sem_destroy(&m_semaphore);/*sem_destroy函数：这个函数是POSIX线程（pthread）库提供的一部分，
    用于销毁一个未使用的信号量。如果信号量正在被等待或计数不是零，调用sem_destroy会导致未定义行为。因此，在调用sem_destroy之前，确保信号量不再被任何线程使用是非常重要的。*/

}

void Semaphore::wait(){
        if(sem_wait(&m_semaphore)){ /*if(!sem_wait(&m_semaphore)) 检查 sem_wait 函数的返回值。
        sem_wait 是 POSIX 线程（pthread）库中用于减少（等待）信号量计数的函数。
        如果信号量的值大于零，sem_wait 会减少其值并返回零（成功）。
        如果信号量的值为零，则调用线程将被阻塞，直到信号量的值大于零。
        注意，sem_wait 在发生错误时返回 -1*/
            throw std::logic_error("sem_wait error");
    }
    
}
void Semaphore::notify(){
    if(sem_post(&m_semaphore)){/*简单来说，sem_post(&m_semaphore); 这行代码的作用就是：
增加信号量 m_semaphore 的值。
如果有线程或进程因为等待该信号量而被阻塞，那么 sem_post 会唤醒其中一个等待的线程或进程，使其可以继续执行。*/
        throw std::logic_error("sem_post error");
    }
    
}

Thread* Thread::GetThis(){
    return t_thread;

}
const std::string& Thread::GetName(){
    return t_thread_name;
    
}

void Thread::SetName(const std::string& name){
    
     if(t_thread){
        t_thread->m_name=name;
     }
     t_thread_name =name;
}

Thread::Thread(std::function<void()> cb ,const std::string& name):m_cb(cb),m_name(name){
    // 如果线程名称为空，则将其设置为"UNKNOW"
    if(name.empty()){
        m_name ="UNKNOW";
    }

    // 创建线程
    int rt =pthread_create(&m_thread,nullptr,&Thread::run,this);
    

    // 如果线程创建失败
    if(rt){
        // 输出错误日志
        SYLAR_LOG_ERROR(g_logger)<<"pthread_create thread fail, rt="<<rt<<"name="
        <<name;

        // 抛出逻辑错误异常
        throw std::logic_error("pthread_create error");
    }
}//std::function<void()> cb：这是一个std::function对象，它被模板化为一个无参数、无返回值的函数签名（void()）。这意味着cb可以是一个普通函数、lambda表达式、函数对象（即重载了operator()的对象）、或者任何其他可调用对象，只要它们满足这个签名（即它们可以接受零个参数并返回一个void类型的值）
Thread::~Thread(){
    if(m_thread){
        pthread_detach(m_thread);
        //用于设置线程的属性，使其变为“分离”状态。
        //这个函数的主要目的是在线程结束时自动回收线程的资源，而不需要用户显式地等待（join）该线程。
    }
    
}

void* Thread::run(void* arg){
    // 将传入的参数转换为Thread类型指针
    Thread* thread =(Thread*)arg;
    // 将线程指针赋值给类的成员变量t_thread
    t_thread =thread;
    // 将线程对象的m_name成员变量赋值给t_thread_name
    t_thread_name=thread->m_name;
    // 获取当前线程的ID，并赋值给线程的m_id成员变量
    thread->m_id = sylar::GetThreadId();
    // 设置当前线程的名字为线程对象的m_name成员变量的前15个字符
    pthread_setname_np(pthread_self(),thread->m_name.substr(0,15).c_str());

    // 声明一个函数对象cb
    std::function<void()> cb;
    // 将线程对象的m_cb成员变量与cb进行交换
    cb.swap(thread->m_cb);
     /*std::function<void()>类型的cb被用作回调函数的容器。
     首先，它通过swap与Thread对象的m_cb成员交换内容。
     m_cb很可能是一个在Thread类中定义的std::function<void()>类型成员，用于存储要在线程中执行的函数。
     之后，cb()被调用，实际执行了存储在m_cb中的函数。*/
    
    // 调用函数对象cb所绑定的函数
    cb();
    return 0;
}


void Thread::join(){
    
    
    if(m_thread){
        int rt= pthread_join(m_thread,nullptr);
        if(rt){
            SYLAR_LOG_ERROR(g_logger)<<"pthread_join thread fail, rt="<<rt<<"name="<<m_name;
        throw std::logic_error("pthread_join error");
        m_thread = 0;
        }

}
}

}