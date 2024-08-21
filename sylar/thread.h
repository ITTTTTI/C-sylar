#ifndef __SYLAR_THREAD_H__
#define __SYLAR_THREAD_H__

#include <thread>
#include <functional>
#include <string>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <atomic>


namespace sylar{

class Semaphore{
public:
     Semaphore(uint32_t count=0);
     ~Semaphore();

     void wait();
     void notify();
private:
     Semaphore(const Semaphore&)=delete;
     Semaphore(const Semaphore&&)=delete;
     Semaphore operator=(const Semaphore&)=delete;
private:
    sem_t m_semaphore;

};

template<class T>
struct ScopedLockImpl{
public:
    ScopedLockImpl(T& mutex):m_mutex(mutex){
        m_mutex.lock();
        m_locked =true;
    }
    ~ScopedLockImpl(){
        unlock();
    }

    void lock(){
        if(!m_locked){
            m_mutex.lock();
            m_locked = true;
        }
    }
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }
 private:   
    T& m_mutex;
    bool m_locked;
};

template<class T>
struct ReadScopedLockImpl{
public:
    ReadScopedLockImpl(T& mutex):m_mutex(mutex){
        m_mutex.rdlock();
        m_locked =true;
    }
    ~ReadScopedLockImpl(){
        unlock();
    }

    void lock(){
        if(!m_locked){
            m_mutex.rdlock();
            m_locked = true;
        }
    }
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }
 private:   
    T& m_mutex;
    bool m_locked;
};

template<class T>
struct WriteScopedLockImpl{
public:
    WriteScopedLockImpl(T& mutex):m_mutex(mutex){
        m_mutex.wrlock();
        m_locked =true;
    }
    ~WriteScopedLockImpl(){
        unlock();
    }

    void lock(){
        if(!m_locked){
            m_mutex.wrlock();
            m_locked = true;
        }
    }
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }
 private:   
    T& m_mutex;
    bool m_locked;
};

class Mutex {
public: 
    /// 局部锁
    typedef ScopedLockImpl<Mutex> Lock;

    /**
     * @brief 构造函数
     */
    Mutex() {
        pthread_mutex_init(&m_mutex, nullptr);
    }

    /**
     * @brief 析构函数
     */
    ~Mutex() {
        pthread_mutex_destroy(&m_mutex);
    }

    /**
     * @brief 加锁
     */
    void lock() {
        pthread_mutex_lock(&m_mutex);
    }

    /**
     * @brief 解锁
     */
    void unlock() {
        pthread_mutex_unlock(&m_mutex);
    }
private:
    /// mutex
    pthread_mutex_t m_mutex;
};

class NullMutex{
    public:
    typedef ScopedLockImpl<NullMutex> Lock;
    NullMutex(){}
    ~NullMutex(){}
    void lock(){}
    void unlock(){}

};

class RWMutex{
public:
   typedef ReadScopedLockImpl<RWMutex> ReadLock;
   typedef WriteScopedLockImpl<RWMutex> WriteLock;    
    RWMutex(){
        // 初始化读写锁
        pthread_rwlock_init(&m_lock,nullptr);
    }
    ~RWMutex(){
        // 销毁读写锁
        pthread_rwlock_destroy(&m_lock);
    }
    void rdlock(){
        pthread_rwlock_rdlock(&m_lock);
    }

    void wrlock(){
        pthread_rwlock_wrlock(&m_lock);
    }

    void unlock(){
        pthread_rwlock_unlock(&m_lock);
    }
private:
   pthread_rwlock_t m_lock;// m_lock 的 pthread_rwlock_t 类型的变量，即一个读写锁
};

class NullRWMutex{
public:
    typedef ReadScopedLockImpl<NullRWMutex> ReadLock;
    typedef WriteScopedLockImpl<NullRWMutex> WriteLock;  
    NullRWMutex(){}
    ~NullRWMutex(){}
    void rdlock(){}
    void wrlock(){}
    void unlock(){}

};

class Spinlock{
public:
typedef ScopedLockImpl<Spinlock> Lock;
    Spinlock(){
        pthread_spin_init(&m_mutex,0);

    }
    ~Spinlock(){
        pthread_spin_destroy(&m_mutex);
    }

    void lock(){
            pthread_spin_lock(&m_mutex);
    }

    void unlock(){
        pthread_spin_unlock(&m_mutex);
    }


private:
  pthread_spinlock_t m_mutex;

};


class CASlock{
public:
typedef ScopedLockImpl<CASlock> Lock;
    CASlock(){
        m_mutex.clear();

    }
    ~CASlock(){
    }

    void lock(){
           while(std::atomic_flag_test_and_set_explicit(&m_mutex,std::memory_order_acquire)){

           }
    }

    void unlock(){
        std::atomic_flag_clear_explicit(&m_mutex,std::memory_order_release);
    }


private:
  volatile std::atomic_flag m_mutex;

};

class Thread{
public:
    typedef std::shared_ptr<Thread> ptr;
    Thread(std::function<void()> cb ,const std::string& name);//std::function<void()> cb：这是一个std::function对象，它被模板化为一个无参数、无返回值的函数签名（void()）。这意味着cb可以是一个普通函数、lambda表达式、函数对象（即重载了operator()的对象）、或者任何其他可调用对象，只要它们满足这个签名（即它们可以接受零个参数并返回一个void类型的值）
    ~Thread();
    
    pid_t getId()const {return m_id;}
    const std::string& getName() const{return m_name;}

    void join();

    static Thread* GetThis();//静态成员函数：static 关键字表明这个函数是静态的，意味着它属于类本身而不是类的某个特定对象实例。
    static const std::string& GetName();
    static void SetName(const std::string& name);

private:
    Thread(const Thread&) = delete;//用于显式地删除拷贝构造函数（Copy Constructor）
    Thread(const Thread&&) = delete;//删除移动构造函数
    Thread& operator=(const Thread&) =delete;//除了拷贝赋值运算符。这意味着你不能将一个Thread对象赋值给另一个Thread对象

    static void* run(void* arg);
private:
    pid_t m_id = -1;
    pthread_t m_thread=0;
    std::function<void()> m_cb;
    std::string m_name;

    Semaphore m_semaphore;

};


}


#endif