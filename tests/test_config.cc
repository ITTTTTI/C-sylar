#include "../sylar/config.h"
#include "../sylar/log.h"
#include <yaml-cpp/yaml.h>

sylar::ConfigVar<int>::ptr g_int_value_config =
    sylar::Config::Lookup("system.port",(int)8080,"system port");

sylar::ConfigVar<float>::ptr g_float_value_config=
   sylar::Config::Lookup("system.value",(float)10.2f,"system value");

sylar::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config = 
   sylar::Config::Lookup("system.int_vec",std::vector<int>{1,2},"system int vec");

sylar::ConfigVar<std::list<int>>::ptr g_int_list_value_config = 
   sylar::Config::Lookup("system.int_list",std::list<int>{1,2},"system int list");

sylar::ConfigVar<std::set<int>>::ptr g_int_set_value_config = 
   sylar::Config::Lookup("system.int_set",std::set<int>{1,2},"system int set");  

sylar::ConfigVar<std::unordered_set<int>>::ptr g_int_uset_value_config = 
   sylar::Config::Lookup("system.int_uset",std::set<int>{1,2},"system int uset");

sylar::ConfigVar<std::map<string,int>>::ptr g_str_int_map_value_config = 
   sylar::Config::Lookup("system.str_int_map",std::map<string,int>{{"k",2}},"system str int map");

sylar::ConfigVar<std::unordered_map<string,int>>::ptr g_str_int_umap_value_config = 
   sylar::Config::Lookup("system.str_int_umap",std::unordered_map<string,int>{{"k",2}},"system str int umap");   


void print_yaml(const YAML::Node& node, int level){
    // 如果节点是Scalar类型
    if(node.IsScalar()){
        // 打印Scalar的值和类型
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<std::string(level*4,' ')<<node.Scalar()<<"-"<<node.Type();
    }
    // 如果节点是Null类型
    else if(node.IsNull()){
        // 打印Null的类型和层级
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<std::string(level*4,' ')<<"NULL -"<<node.Type()<<" - "<<level;
    }
    // 如果节点是Map类型
    else if(node.IsMap()){
        // 遍历Map中的每个键值对
        for(auto it = node.begin();it!=node.end();++it){
            // 打印键、值的类型和层级
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<std::string(level*4,' ')<<
            it->first<<"-"<<it->second.Type()<<" - "<<level;
            // 递归打印值的子节点
            print_yaml(it->second,level+1);
        }
    }
    // 如果节点是Sequence类型
    else if(node.IsSequence()){
        // 遍历Sequence中的每个元素
        for(size_t i=0; i<node.size();++i){
            // 打印元素的索引、类型和层级
             SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<std::string(level*4,' ')
             <<i<<"-"<<node[i].Type()<<" - "<<level;
            // 递归打印元素的子节点
         print_yaml(node[i],level+1);
        }

    }

}

void test_yaml(){
    YAML::Node root = YAML::LoadFile("/home/flysee/sylar/workspace/sylar/bin/conf/log.yml");
    print_yaml(root,0);
    //SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<root;
}

void test_config(){
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"before:"<<g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"before:"<<g_float_value_config->getValue();

#define XX(g_var,name,prefix)\
    {\
        auto& v =g_var->getValue();\
    for(auto& i:v){\
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<#prefix " " #name ":"<<i;\
    }\
         SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<#prefix " " #name ":"<<g_var->toString();\;
    }

#define XX_M(g_var,name,prefix)\
    {\
        auto& v =g_var->getValue();\
    for(auto& i:v){\
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<#prefix " " #name ":"<<i.first<<"-"<<i.second<<"}";\
    }\
         SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<#prefix " " #name ":"<<g_var->toString();\;
    }    

    XX   (g_int_vec_value_config,"int_vec","before"); 
    XX   (g_int_list_value_config,"int_list","before"); 
    XX   (g_int_set_value_config,"int_set","before"); 
    XX   (g_int_uset_value_config,"int_uset","before"); 
    XX_M   (g_str_int_map_value_config,"str_int_map","before"); 
    XX_M   (g_str_int_umap_value_config,"str_int_umap","before");
    

    YAML::Node root=YAML::LoadFile("/home/flysee/sylar/workspace/sylar/bin/conf/log.yml");
    sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"before:"<<g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"before:"<<g_float_value_config->getValue();
    XX  (g_int_vec_value_config,"int_vec","after");
    XX  (g_int_list_value_config,"int_list","after");
    XX  (g_int_set_value_config,"int_set","after");
    XX  (g_int_uset_value_config,"int_uset","after");
    XX_M   (g_str_int_map_value_config,"str_int_map","after"); 
    XX_M   (g_str_int_umap_value_config,"str_int_umap","after"); 
}

int main(int argc, char** argv){
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<< g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<< g_int_value_config->toString();

    test_yaml();
    return 0;
}