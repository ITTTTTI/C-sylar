#ifndef __SYLAR_SINGLETON_H__ //头文件防护，用于防止头文件被多次包含
#define __SYLAR_SINGLETON_H__
namespace sylar{
    template<class T, class X = void ,int N=0>
class Singleton{
public:
    static T* GetInstance(){
        // 静态局部变量v，在函数第一次被调用时初始化
        static T v;
        // 返回v的地址
        return &v;
    }
};

template<class T, class X = void , int N=0>
class SingletonPtr{
    public:
    static std::shared_ptr<T> GetInstance(){
        // 静态局部变量，只在第一次调用时创建，之后每次调用都会返回相同的指针
        static std::shared_ptr<T> v(new T);
        // 返回创建的实例的共享指针
        return v;
    }
};
}
#endif