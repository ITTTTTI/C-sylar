#ifndef __SYLAR_TIMER_H__
#define __SYLAR_TIMER_H__

#include <memory>
#include "thread.h"
#include <functional>
#include <vector>
#include <set>
namespace sylar {
class TimerManager;
class Timer : public std::enable_shared_from_this<Timer> { 
friend class TimerManager;
    //std::enable_shared_from_this,支持从 std::shared_ptr 管理的对象内部创建额外的 std::shared_ptr 实例，
    //同时确保这些新的 std::shared_ptr 实例正确地与原始 std::shared_ptr 实例共享所有权
public:
    typedef std::shared_ptr<Timer> ptr;

private:
    Timer(uint64_t ms, std::function<void()> cb,
        bool recurring, TimerManager* manager);
        //构造函数私有，外部无法创建Timer对象，只能通过TimerManager的addTimer来创建
    Timer(uint64_t next);

    bool cancel();

private:
    bool m_recurring =false;   //是否循环定时器
    uint64_t m_ms = 0;        //执行周期
    uint64_t m_next =0;    //精确的执行时间点
    std::function<void()> m_cb;
    TimerManager* m_manager =nullptr;

private:
struct Comparator {
    bool operator() (const Timer::ptr& lhs, const Timer::ptr& rhs)const;
};
};

class TimerManager{
friend class Timer;
public:
    typedef RWMutex RWMutexType;
    TimerManager();
    virtual ~TimerManager();

    Timer::ptr addTimer(uint64_t ms, std::function<void()> cb
                        ,bool recuring =false); //添加定时器

    Timer::ptr addConditionTimer(uint64_t ms,std::function<void()> cb,
                                 std::weak_ptr<void> weak_cond
                                 ,bool recurring =false);
    uint64_t getNextTimer();
    void listExpiredCb(std::vector<std::function<void>>& cbs);
protected:
    virtual void onTimerInsertedAtfront()=0;
private:
    RWMutexType m_mutex;
    std::set<Timer::ptr,Timer::Comparator> m_timers;
    //自定义的规则来排序 std::set 中的元素，你可以提供一个比较函数或函数对象给 std::set 的模板参数
};

}

#endif