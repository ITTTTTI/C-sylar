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
#include<unorderd_map>
#include<unorderd_set>

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
    virtual std::string getTypeName() const =0
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
class LexicalCast<std:string,std::vector<T>>{
    public:
    std::list<T> operator()(const std::string& v){
        YAML::Node node =YAML::Load(v);
        typename std::vector<T> vec;
        std::stringstream ss;
        for(size_t i=0; i<node.size();++i){
            ss.str("");//ss中的内容清空。
            ss<<node[i];//node[i]的值插入到std::stringstream对象ss中,node是一个容器（如std::vector、std::array、std::list等），并且它包含可以转换为字符串的类型（如int、double、std::string等），那么node[i]将取出容器中索引为i的元素，并将其转换为字符串（如果它不是字符串类型的话），然后将这个字符串插入到ss中
            vec.push_back(LexicalCast<std::strin,T>()(ss.str()));
        }
        return vec;
    }
    
};

template<class T>
class LexicalCast<std::vector<T>,std:string>{
    public:
    std::sting operator()(const std::vector<T>& v){
        YAML::Node node ;
        for(auto& i :v){
            node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
        }
        std::stringstream ss;
        ss<<node;
        reeturn ss.str();
    }


};

template<class T>
class LexicalCast<std:string,std::list<T>>{
    public:
    std::list<T> operator()(const std::string& v){
        YAML::Node node =YAML::Load(v);
        typename std::list<T> vec;
        std::stringstream ss;
        for(size_t i=0; i<node.size();++i){
            ss.str("");//ss中的内容清空。
            ss<<node[i];//node[i]的值插入到std::stringstream对象ss中,node是一个容器（如std::vector、std::array、std::list等），并且它包含可以转换为字符串的类型（如int、double、std::string等），那么node[i]将取出容器中索引为i的元素，并将其转换为字符串（如果它不是字符串类型的话），然后将这个字符串插入到ss中
            vec.push_back(LexicalCast<std::strin,T>()(ss.str()));
        }
        return vec;
    }
    
};

template<class T>
class LexicalCast<std::list<T>,std:string>{
    public:
    std::sting operator()(const std::list<T>& v){
        YAML::Node node ;
        for(auto& i :v){
            node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
        }
        std::stringstream ss;
        ss<<node;
        reeturn ss.str();
    }
};

template<class T>
class LexicalCast<std:string,std::set<T>>{
    public:
    std::set<T> operator()(const std::string& v){
        YAML::Node node =YAML::Load(v);
        typename std::set<T> vec;
        std::stringstream ss;
        for(size_t i=0; i<node.size();++i){
            ss.str("");//ss中的内容清空。
            ss<<node[i];//node[i]的值插入到std::stringstream对象ss中,node是一个容器（如std::vector、std::array、std::list等），并且它包含可以转换为字符串的类型（如int、double、std::string等），那么node[i]将取出容器中索引为i的元素，并将其转换为字符串（如果它不是字符串类型的话），然后将这个字符串插入到ss中
            vec.insert(LexicalCast<std::strin,T>()(ss.str()));
        }
        return vec;
    }
    
};

template<class T>
class LexicalCast<std::set<T>,std:string>{
    public:
    std::sting operator()(const std::set<T>& v){
        YAML::Node node ;
        for(auto& i :v){
            node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
        }
        std::stringstream ss;
        ss<<node;
        reeturn ss.str();
    }
};

template<class T>
class LexicalCast<std:string,std::unordered_set<T>>{
    public:
    std::unordered_set<T> operator()(const std::string& v){
        YAML::Node node =YAML::Load(v);
        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for(size_t i=0; i<node.size();++i){
            ss.str("");//ss中的内容清空。
            ss<<node[i];//node[i]的值插入到std::stringstream对象ss中,node是一个容器（如std::vector、std::array、std::list等），并且它包含可以转换为字符串的类型（如int、double、std::string等），那么node[i]将取出容器中索引为i的元素，并将其转换为字符串（如果它不是字符串类型的话），然后将这个字符串插入到ss中
            vec.insert(LexicalCast<std::strin,T>()(ss.str()));
        }
        return vec;
    }
    
};

template<class T>
class LexicalCast<std::unordered_set<T>,std:string>{
    public:
    std::sting operator()(const std::unordered_set<T>& v){
        YAML::Node node ;
        for(auto& i :v){
            node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
        }
        std::stringstream ss;
        ss<<node;
        reeturn ss.str();
    }
};


template<class T>
class LexicalCast<std:string,std::map<std::string,T>>{
    public:
    std::map<std::string,T> operator()(const std::string& v){
        YAML::Node node =YAML::Load(v);
        typename std::map<std::string,T> vec;
        std::stringstream ss;
        for(auto it=node.begin();it!=node.end();++it){
            ss.str("");//ss中的内容清空。
            ss<<it->second;//node[i]的值插入到std::stringstream对象ss中,node是一个容器（如std::vector、std::array、std::list等），并且它包含可以转换为字符串的类型（如int、double、std::string等），那么node[i]将取出容器中索引为i的元素，并将其转换为字符串（如果它不是字符串类型的话），然后将这个字符串插入到ss中
            vec.insert(std::make_pair(it->first.Scalar(),LexicalCast<std::strin,T>()(ss.str()))); //Scalar() 方法通常用于获取矩阵或向量的元素类型
        }
        return vec;
    }
    
};

template<class T>
class LexicalCast<std::map<std::string,T>,std:string>{
    public:
    std::sting operator()(const std::map<std::string,T>& v){
        YAML::Node node ;
        for(auto& i :v){
            node[i.first]=YAML::Load(LexicalCast<T,std::string>()(i.second))
            node.push_back();
        }
        std::stringstream ss;
        ss<<node;
        reeturn ss.str();
    }
};

template<class T>
class LexicalCast<std:string,std::unordered_map<std::string,T>>{
    public:
    std::map<std::string,T> operator()(const std::string& v){
        YAML::Node node =YAML::Load(v);
        typename std::unordered_map<std::string,T> vec;
        std::stringstream ss;
        for(auto it=node.begin();it!=node.end();++it){
            ss.str("");//ss中的内容清空。
            ss<<it->second;//node[i]的值插入到std::stringstream对象ss中,node是一个容器（如std::vector、std::array、std::list等），并且它包含可以转换为字符串的类型（如int、double、std::string等），那么node[i]将取出容器中索引为i的元素，并将其转换为字符串（如果它不是字符串类型的话），然后将这个字符串插入到ss中
            vec.insert(std::make_pair(it->first.Scalar(),LexicalCast<std::strin,T>()(ss.str()))); //Scalar() 方法通常用于获取矩阵或向量的元素类型
        }
        return vec;
    }
    
};

template<class T>
class LexicalCast<std::unordered_map<std::string,T>,std:string>{
    public:
    std::sting operator()(const std::unordered_map<std::string,T>& v){
        YAML::Node node ;
        for(auto& i :v){
            node[i.first]=YAML::Load(LexicalCast<T,std::string>()(i.second))
            node.push_back();
        }
        std::stringstream ss;
        ss<<node;
        reeturn ss.str();
    }
};

template<class T, class FromStr=LexicalCast<std::string,T>
, class ToStr=LexicalCast<T,std::string>>
class ConfigVar : public ConfigVarBase{
    public:
       typedef std::shared_ptr<ConfigVar> ptr;
  
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
        const T getValue() const{return m_val;}
        void setValue(const T& v){m_val=v;}
        std::string getTypeName() const override {return typeid(T).name();}//只读函数，const表示这个函数不会修改类的任何非静态成员变量（除非它们被声明为mutable）。override关键字表示这个函数在基类中有一个虚函数声明，它在这里被重写（override
    private:
       T m_val;
};


class Config {
    public:
    typedef std ::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;
    template<class T>
    //typename 关键字告诉编译器，ConfigVar<T>::ptr 是一个类型
    static typename ConfigVar<T>::ptr Lookup( const std::string& name,const T& default_value,
        const std::string& description = ""
    ){
        auto it =s_datas.find(name);
        if(it!=s_datas.end()){
           auto tmp=std::dynamic_pointer_cast<ConfigVar<T>> (it->second);
           if(tmp){
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"Lookup name="<<name<<"exists";
            return tmp;
        }else {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"Lookup name="<<name<<"exists but type not"<<typeid(T).name()<<"real_type="<<it->second->getTypeName()<<" "<<it->second->toString();
            return nullptr;
        }
        }
        
        if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678")!=
        std::string::npos){
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"Lookup name invalid "<<name;
                throw std::invalid_argument(name);
            
        }
        typename ConfigVar<T>::ptr v(new ConfigVar<T>(name,default_value,description));
        s_datas[name]=v;
        return v;

    }

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string& name){
         auto it = s_datas.find(name);
         if(it==s_datas.end()){
            return nullptr;
         }
         return std::dynamic_pointer_cast<ConfigVar<T>> (it->second);//将一个 std::shared_ptr 类型的指针安全地转换为另一个类型的 std::shared_ptr 指针
    }

    static void LoadFromYaml(const YAML::Node&root);
   
    static ConfigVarBase::ptr LookupBase(const std::string& name);
    private:
    static ConfigVarMap s_datas;
};
} 



#endif