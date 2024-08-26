#ifndef __SYLAR_SCHEDULER_H__
#define __SYLAR_SCHEDULER_H__

#include <memory>
#include "thread.h"
#include "fiber.h"
#include <list>
#include <vector>

namespace sylar{
class Scheduler{
public:
    typedef std::shared_ptr<Scheduler> ptr;
    typedef Mutex MutexType;

    Scheduler(size_t threads =1, bool use_caller =true, const std::string& name="");
    virtual ~Scheduler();

    const std::string& getName() const{return m_name;}

    static Scheduler* GetThis();
    static Fiber* GetMainFiber(); 

    void start();
    void stop(); //

    template<class FiberOrCb>
    void schedule(FiberOrCb fc, int thread =-1){
        bool need_tickle=false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle= scheduleNoLock(fc,thread);
        }
        if(need_tickle){
            tickle();
        }
    }

    template<class InputIterator>
    void schedule(InputIterator begin, InputIterator end){
        bool need_tickle=false;
        {
            MutexType::Lock lock(m_mutex);
            while(begin!=end){
                tickle=scheduleNoLock(&*begin)||need_tickle;

            }
        }
        if(need_tickle){
            tickle();
        }
    }

protected:
   virtual void tickle();
   void run();
   virtual bool stopping();
   virtual void idle();
   void setThis();
private:
    template<class FiberOrCb>
    bool scheduleNoLock(FiberOrCb fc, int thread){
        bool need_tickle=m_fibers.empty();
        FiberAndThread ft(fc,thread);
        if(ft.fiber || ft.cb){
            m_fibers.push_back(ft);
        }
        return need_tickle;
    }
private:
    struct  FiberAndThread
    {
        Fiber::ptr fiber;
        std::function<void()> cb;
        int thread_id;

        FiberAndThread(Fiber::ptr f, int thr):fiber(f),thread_id(thr){
        }

        FiberAndThread(Fiber::ptr* f, int thr):thread_id(thr){
            fiber.swap(*f);
        }

        FiberAndThread(std::function<void()> f,int thr):cb(f),thread_id(thr){

        }

        FiberAndThread(std::function<void()>* f,int thr):thread_id(thr){
            cb.swap(*f);
        }

        FiberAndThread():thread_id(-1){
        }

        void reset(){
            fiber=nullptr;
            cb=nullptr;
            thread_id =-1;
        }

    };
    
private:
    MutexType m_mutex;
    std::vector<Thread::ptr> m_threads; //线程池
    std::list<FiberAndThread> m_fibers;//任务队列
    std::string m_name; //名字
    Fiber::ptr m_rootFiber; //主fiber

protected:
    std::vector<int> m_threadIds;//线程id
    size_t m_threadCount=0;  //线程数
    std::atomic<size_t> m_activeThreadCount={0}; //活跃线程数
    std::atomic<size_t> m_idleThreadCount={0}; //空闲线程数
    bool m_stopping=true; //停止标志
    bool m_autoStop=false; //自动停止
    int m_rootThread=0;  //主线程id
};

}

#endif