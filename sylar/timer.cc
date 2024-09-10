#include "timer.h"
#include "sylar.h"

namespace sylar {
bool Timer::Comparator::operator() (const Timer::ptr& lhs, const Timer::ptr& rhs)const{
    if(!lhs && !rhs) {
        return false;
    }

    if(!lhs) {
        return true;
    }
    if(!rhs){
        return false;
    }
    if(lhs->m_next<rhs->m_next){
        return true;
    }
    if(rhs->m_next<lhs->m_next){
        return false;
    }
    return lhs.get()<rhs.get();
}

Timer::Timer(uint64_t ms, std::function<void()> cb,bool recurring, TimerManager* manager):
     m_recurring(recurring), 
     m_ms(ms),
     m_cb(cb),
     m_manager(manager){
     m_next =sylar::GetCurrentMS()+m_ms;

    }

Timer::Timer(uint64_t next):m_next(next){

}

bool Timer::cancel(){
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(m_cb){
        m_cb=nullptr;
        auto it =m_manager->m_timers.find(shared_from_this());
        m_manager->m_timers.erase(it);
        return true;
    }
    return false;
}

bool Timer::refresh(){
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb)
        return false;
    auto it =m_manager->m_timers.find(shared_from_this());
    if(it!=m_manager->m_timers.end()){
        return false;
    }
    m_manager->m_timers.erase(it);
    m_next=sylar::GetCurrentMS()+m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}
bool Timer::reset(uint64_t ms, bool from_now){
    if(ms==m_ms&&!from_now){
        return true;
    }
    TimerManager::RWMutexType::WriteLock lock(m_manager->m_mutex);
    if(!m_cb)
        return false;
    auto it =m_manager->m_timers.find(shared_from_this());
    if(it==m_manager->m_timers.end()){
        return false;
    }
    m_manager->m_timers.erase(it);
    uint64_t start =0;
    if(from_now){
        start =sylar::GetCurrentMS();
    }else{
        start=m_next-m_ms;
    }
    m_ms=ms;
    m_next=start+m_ms;
    m_manager->addTimer(shared_from_this(),lock);
    return true;

}

TimerManager::TimerManager(){
    m_previouseTime =sylar::GetCurrentMS();
    
}
TimerManager::~TimerManager(){
    
}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> cb,bool recuring)                   
{
      Timer::ptr timer(new Timer(ms,cb,recuring,this));
      RWMutexType::WriteLock lock(m_mutex);
      addTimer(timer,lock);
      return timer;

}

bool TimerManager::hasTimer(){
    RWMutexType::ReadLock lock(m_mutex);
    return !m_timers.empty();

}

static void OnTimer(std::weak_ptr<void> weak_cond, std::function<void()> cb){
    std::shared_ptr<void> tmp =weak_cond.lock();
    //如果weak_cond仍然指向一个有效的对象（即该对象尚未被销毁），
    //则lock()会返回一个指向该对象的std::shared_ptr<void>；
    //否则，如果weak_cond不再指向任何对象
    if(tmp){
        cb();
    }
}

Timer::ptr TimerManager::addConditionTimer(uint64_t ms,std::function<void()> cb,
                                std::weak_ptr<void> weak_cond
                                ,bool recurring)
{ 
    return addTimer(ms, std::bind(&OnTimer, weak_cond, cb), recurring);
                                    
}

uint64_t TimerManager::getNextTimer(){
    RWMutexType::ReadLock lock(m_mutex);
    m_tickled = false;
    if(m_timers.empty()){
        return ~0ull;
    }

    const Timer::ptr& next =*m_timers.begin();
    uint64_t now_ms=sylar::GetCurrentMS();
    if(now_ms>=next->m_next){
        return 0;
    }else{
        return next->m_next-now_ms;
    }
}

void TimerManager::listExpiredCb(std::vector<std::function<void()>>& cbs){
      uint64_t now_ms =sylar::GetCurrentMS();
      std::vector<Timer::ptr> expired;
      {
        RWMutexType::ReadLock lock(m_mutex);
        if(m_timers.empty()){
            return;
        }
      }
      RWMutexType::WriteLock lock(m_mutex);

      bool rollover =detectClockRollover(now_ms);
      if(!rollover && ((*m_timers.begin())->m_next==now_ms)){
          return;
      }

      Timer::ptr now_timer(new Timer(now_ms));
      auto it = rollover? m_timers.end(): m_timers.lower_bound(now_timer);
      //lower_bound 是一个定义在 <algorithm> 头文件中的函数模板，
      //它用于在一个已排序的范围内查找不小于（即大于或等于）给定值的第一个元素的迭代器。
      while(it!=m_timers.end()&& ((*it)->m_next==now_ms)){
          ++it;
      }
      expired.insert(expired.begin(),m_timers.begin(),it);
      m_timers.erase(m_timers.begin(),it);
      cbs.reserve(expired.size());

      for(auto& timer:expired){
        cbs.push_back(timer->m_cb);
        if(timer->m_recurring){
            timer->m_next=now_ms + timer->m_ms;
            m_timers.insert(timer);
        }else{
            timer->m_cb=nullptr;
        }

      }
}

void TimerManager::addTimer(Timer::ptr val,RWMutexType::WriteLock& lock){
    auto it = m_timers.insert(val).first;
    //insert函数返回一个std::pair<iterator, bool>，
    //其中iterator是一个指向新插入元素（或已存在的相等元素）的迭代器，
    //而bool值表示是否成功插入了新元素（如果集合中已存在具有相同值的元素，则为false）。
    bool at_front = (it== m_timers.begin())&& !m_tickled;
    if(at_front){
            m_tickled=true;
    }
    lock.unlock();

    if(at_front){
    onTimerInsertedAtfront();
    }
}

bool TimerManager::detectClockRollover(uint64_t now_ms){
    bool rollover =false;
    if(now_ms<m_previouseTime&& now_ms<(m_previouseTime-60*60*1000)){
        rollover=true;
    }
    m_previouseTime =now_ms;
    return rollover;
}

}