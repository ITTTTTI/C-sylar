#ifndef __SYLAR_CONFIG_H__
#define __SYLAR_CONFIG_H__

#include <memory>
#include <string> 
#include <sstream>
#include <boost/lexical_cast.hpp>
#include "log.h"
#include <yaml-cpp/yaml.h>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include "thread.h"
#include "log.h"

namespace sylar
{
class ConfigVarBase{
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;
    ConfigVarBase(const std::string& name, const std::string& description ="")
    :m_name(name)
    ,m_description(description){
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);

    }
    virtual ~ConfigVarBase(){
    }
    const std::string& getName() const { return m_name;}
    const std::string& getDescription() const {return m_description;}

    virtual std::string toString() = 0;//纯虚函数是那些只有函数声明而没有函数实现的成员函数，它们必须在任何派生类
    virtual bool fromString(const std::string& val) =0;
    virtual std::string getTypeName() const =0;

protected:
    std::string m_name;
    std::string m_description;
};
//F fromm_type, T to_type
//允许对模板类或模板函数的某些特定实例进行定制化，而不是对所有可能的模板参数进行完全特化
template<class F,class T>
class LexicalCast{
public:
   T operator()(const F& v){
    return boost::lexical_cast<T>(v);
   }
};




template<class T>
class LexicalCast< std::string ,std::vector<T>>{
    public:
    std::vector<T> operator()(const std::string& v){
        YAML::Node node =YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream ss;
        for(size_t i=0; i<node.size();++i){
            ss.str("");//ss中的内容清空。
            ss<<node[i];//node[i]的值插入到std::stringstream对象ss中,node是一个容器（如std::vector、std::array、std::list等），并且它包含可以转换为字符串的类型（如int、double、std::string等），那么node[i]将取出容器中索引为i的元素，并将其转换为字符串（如果它不是字符串类型的话），然后将这个字符串插入到ss中
            vec.push_back(LexicalCast<std::string,T>()(ss.str()));
        }
        return vec;
    }
    
};

template<class T>
class LexicalCast<std::vector<T>,std::string>{
    public:
    std::string operator()(const std::vector<T>& v){
        YAML::Node node(YAML::NodeType::Sequence);
        for(auto& i :v){
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss<<node;
        return ss.str();
    }


};

template<class T>
class LexicalCast<std::string,std::list<T>>{
    public:
    std::list<T> operator()(const std::string& v){
        YAML::Node node =YAML::Load(v);
        typename std::list<T> vec;
        std::stringstream ss;
        for(size_t i=0; i<node.size();++i){
            ss.str("");//ss中的内容清空。
            ss<<node[i];//node[i]的值插入到std::stringstream对象ss中,node是一个容器（如std::vector、std::array、std::list等），并且它包含可以转换为字符串的类型（如int、double、std::string等），那么node[i]将取出容器中索引为i的元素，并将其转换为字符串（如果它不是字符串类型的话），然后将这个字符串插入到ss中
            vec.push_back(LexicalCast<std::string,T>()(ss.str()));
        }
        return vec;
    }
    
};

template<class T>
class LexicalCast<std::list<T>,std::string>{
    public:
    std::string operator()(const std::list<T>& v){
        YAML::Node node ;
        for(auto& i :v){
            node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
        }
        std::stringstream ss;
        ss<<node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string,std::set<T>>{
    public:
    std::set<T> operator()(const std::string& v){
        YAML::Node node =YAML::Load(v);
        typename std::set<T> vec;
        std::stringstream ss;
        for(size_t i=0; i<node.size();++i){
            ss.str("");//ss中的内容清空。
            ss<<node[i];//node[i]的值插入到std::stringstream对象ss中,node是一个容器（如std::vector、std::array、std::list等），并且它包含可以转换为字符串的类型（如int、double、std::string等），那么node[i]将取出容器中索引为i的元素，并将其转换为字符串（如果它不是字符串类型的话），然后将这个字符串插入到ss中
            vec.insert(LexicalCast<std::string,T>()(ss.str()));
        }
        return vec;
    }
    
};

template<class T>
class LexicalCast<std::set<T>,std::string>{
    public:
    std::string operator()(const std::set<T>& v){
        YAML::Node node ;
        for(auto& i :v){
            node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
        }
        std::stringstream ss;
        ss<<node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string,std::unordered_set<T>>{
    public:
    std::unordered_set<T> operator()(const std::string& v){
        YAML::Node node =YAML::Load(v);
        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for(size_t i=0; i<node.size();++i){
            ss.str("");//ss中的内容清空。
            ss<<node[i];//node[i]的值插入到std::stringstream对象ss中,node是一个容器（如std::vector、std::array、std::list等），并且它包含可以转换为字符串的类型（如int、double、std::string等），那么node[i]将取出容器中索引为i的元素，并将其转换为字符串（如果它不是字符串类型的话），然后将这个字符串插入到ss中
            vec.insert(LexicalCast<std::string,T>()(ss.str()));
        }
        return vec;
    }
    
};

template<class T>
class LexicalCast<std::unordered_set<T>,std::string>{
    public:
    std::string operator()(const std::unordered_set<T>& v){
        YAML::Node node ;
        for(auto& i :v){
            node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
        }
        std::stringstream ss;
        ss<<node;
        return ss.str();
    }
};


template<class T>
class LexicalCast<std::string,std::map<std::string,T>>{
    public:
    std::map<std::string,T> operator()(const std::string& v){
        YAML::Node node =YAML::Load(v);
        typename std::map<std::string,T> vec;
        std::stringstream ss;
        for(auto it=node.begin();it!=node.end();++it){
            ss.str("");//ss中的内容清空。
            ss<<it->second;//node[i]的值插入到std::stringstream对象ss中,node是一个容器（如std::vector、std::array、std::list等），并且它包含可以转换为字符串的类型（如int、double、std::string等），那么node[i]将取出容器中索引为i的元素，并将其转换为字符串（如果它不是字符串类型的话），然后将这个字符串插入到ss中
            vec.insert(std::make_pair(it->first.Scalar(),LexicalCast<std::string,T>()(ss.str()))); //Scalar() 方法通常用于获取矩阵或向量的元素类型
        }
        return vec;
    }
    
};

template<class T>
class LexicalCast<std::map<std::string,T>,std::string>{
    public:
    std::string operator()(const std::map<std::string,T>& v){
        YAML::Node node ;
        for(auto& i :v){
            node[i.first]=YAML::Load(LexicalCast<T,std::string>()(i.second));
        }
        std::stringstream ss;
        ss<<node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string,std::unordered_map<std::string,T>>{
    public:
    std::unordered_map<std::string,T> operator()(const std::string& v){
        YAML::Node node =YAML::Load(v);
        typename std::unordered_map<std::string,T> vec;
        std::stringstream ss;
        for(auto it=node.begin();it!=node.end();++it){
            ss.str("");//ss中的内容清空。
            ss<<it->second;//node[i]的值插入到std::stringstream对象ss中,node是一个容器（如std::vector、std::array、std::list等），并且它包含可以转换为字符串的类型（如int、double、std::string等），那么node[i]将取出容器中索引为i的元素，并将其转换为字符串（如果它不是字符串类型的话），然后将这个字符串插入到ss中
            vec.insert(std::make_pair(it->first.Scalar(),LexicalCast<std::string,T>()(ss.str()))); //Scalar() 方法通常用于获取矩阵或向量的元素类型
        }
        return vec;
    }
    
};

template<class T>
class LexicalCast<std::unordered_map<std::string,T>,std::string>{
    public:
    std::string operator()(const std::unordered_map<std::string,T>& v){
        YAML::Node node ;
        for(auto& i :v){
            node[i.first]=YAML::Load(LexicalCast<T,std::string>()(i.second));
        }
        std::stringstream ss;
        ss<<node;
        return ss.str();
    }
};

template<class T, class FromStr=LexicalCast<std::string,T>
, class ToStr=LexicalCast<T,std::string>>
class ConfigVar : public ConfigVarBase{
    public:
       typedef RWMutex RWMutexType;
       typedef std::shared_ptr<ConfigVar> ptr;
       typedef std::function<void (const T& old_value, const T& new_value)> on_change_cb;
  
       ConfigVar(const std::string& name, const T& default_value, const std::string&description ="")
       // 配置变量的描述信息，默认为空字符串
       :ConfigVarBase(name, description)
       // 调用基类ConfigVarBase的构造函数，传入名称和描述信息
       ,m_val(default_value)
       // 初始化成员变量m_val，值为传入的default_value
       {
       }
       std::string toString() override{
        // 尝试将 m_val 转换为 std::string 类型
        try {
             RWMutexType::ReadLock Lock(m_mutex);
            //   return boost::lexical_cast<std::string>(m_val);
              return ToStr()(m_val);
        // 如果转换失败，则捕获所有异常（这里并未对异常进行处理）
        } catch(std::exception& e){
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"ConfigVar::toString exception"<<e.what()<<
            "convert: "<< typeid(m_val).name()<<"to string";

        }
        // 无论是否捕获到异常，都返回空字符串
        return "";
       }
       bool fromString (const std::string& val) override{
              try{
                 // 将字符串 val 转换为类型 T，并赋值给 m_val
                 //m_val=boost::lexical_cast<T> (val);
                 setValue(FromStr()(val));

              }catch (std::exception& e){
                 // 捕获异常，并打印错误信息
                 // 打印日志，记录异常信息
                 SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"ConfigVar::toString exception"<<e.what()<<
            "convert: string to"<< typeid(m_val).name();
              }
              // 始终返回 true，表示函数执行成功
              return true;
       }
        const T getValue() {
            RWMutexType::ReadLock Lock(m_mutex);
            return m_val;
            }
        void setValue(const T& v){
            {
                RWMutexType::ReadLock lock(m_mutex);
            // 如果传入的值等于成员变量m_val的值
            if(v==m_val){
                // 直接返回，不进行后续操作
                return;
            }

            // 遍历回调函数列表m_cbs
            for(auto& i:m_cbs){
                // 调用回调函数i，传入旧值m_val和新值v
                i.second(m_val,v);
            }
            }
            RWMutexType::WriteLock lock(m_mutex);

            // 更新成员变量m_val的值为传入的值v
            m_val=v;
        }
        std::string getTypeName() const override {return typeid(T).name();}//只读函数，const表示这个函数不会修改类的任何非静态成员变量（除非它们被声明为mutable）。override关键字表示这个函数在基类中有一个虚函数声明，它在这里被重写（override
        
        uint64_t addListener(on_change_cb cb){
            static uint64_t s_dun_id=0;
            RWMutexType::WriteLock lock(m_mutex);
            ++s_dun_id;
            m_cbs[s_dun_id]=cb;
            return s_dun_id;
        }
        void delListener(u_int64_t key){
            RWMutexType::WriteLock lock(m_mutex);
            m_cbs.erase(key);

        }

        void clearListener(){
            m_cbs.clear();

        }

        on_change_cb getListener(uint64_t key){
            RWMutexType::ReadLock lock(m_mutex);
          auto it =m_cbs.find(key);
          return it==m_cbs.end()?nullptr:it->second;
        }

    private:
       RWMutexType m_mutex;
       T m_val;
       std::map<uint64_t,on_change_cb> m_cbs;
};


class Config {
    public:
    typedef std ::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;
    typedef RWMutex RWMutexType;
    template<class T>
    //typename 关键字告诉编译器，ConfigVar<T>::ptr 是一个类型
    static typename ConfigVar<T>::ptr Lookup( const std::string& name,const T& default_value,
        const std::string& description = ""
    ){
        RWMutexType::WriteLock lock(GetMutex());
        // 在GetDatas()中查找name对应的迭代器
        auto it = GetDatas().find(name);
        // 如果找到了
        if(it!=GetDatas().end()){
           // 将迭代器指向的值转换为ConfigVar<T>类型的智能指针
           auto tmp=std::dynamic_pointer_cast<ConfigVar<T>> (it->second);
           if(tmp){
            // 打印日志，表示找到了name对应的值
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"Lookup name="<<name<<"exists";
            // 返回找到的值
            return tmp;
        }else {
            // 打印错误日志，表示找到了name但类型不匹配
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"Lookup name="<<name<<"exists but type not"<<typeid(T).name()<<"real_type="<<it->second->getTypeName()<<" "<<it->second->toString();
            // 返回空指针
            return nullptr;
        }
        }

        // 检查name是否只包含小写字母、数字、点号和下划线
        if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678")!=
        std::string::npos){
            // 如果name无效，打印错误日志
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"Lookup name invalid "<<name;
            // 抛出异常
            throw std::invalid_argument(name);
        }

        // 创建一个新的ConfigVar<T>对象，并初始化name、default_value和description
        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name,default_value,description));
        // 将新创建的对象添加到GetDatas()中
        GetDatas()[name]=v;
        // 返回新创建的对象
        return v;
    }

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name){
        RWMutexType::ReadLock lock(GetMutex());
         auto it = GetDatas().find(name);
         if(it==GetDatas().end()){
            return nullptr;
         }
         return std::dynamic_pointer_cast<ConfigVar<T>> (it->second);//将一个 std::shared_ptr 类型的指针安全地转换为另一个类型的 std::shared_ptr 指针
    }

    static void LoadFromYaml(const YAML::Node&root);
   
    static ConfigVarBase::ptr LookupBase(const std::string& name);
    static void Visit(std::function<void(ConfigVarBase::ptr)> cb); //std::function是C++标准库中的一个模板类，它提供了一种通用的方式来存储、复制和调用任何可调用目标
    private:
    static ConfigVarMap& GetDatas(){
        static ConfigVarMap s_datas;
        return s_datas;

    }
    static RWMutexType& GetMutex(){//RWMutexType& 表示这个函数返回一个对 RWMutexType 类型对象的引用
        static RWMutexType s_mutex;
        return s_mutex;

    }
    
};
} 



#endif