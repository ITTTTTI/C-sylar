#include "../sylar/config.h"


namespace sylar {
    Config::ConfigVarMap Config::s_datas;
    ConfigVarBase::ptr Config::LookupBase(const std::string& name){
        auto it = s_datas.find(name);
        return it==s_datas.end()?nullptr:it->second;

    }

    //"A.B",10
    //A:
    //  B:10
    //  C:str
    static void ListAllMember(const std::string &prefix,
                              const YAML::Node& node,
                              std::list<std::pair<std::string,const YAML::Node>>&output){
        // 检查前缀是否只包含小写字母、数字、下划线和点
        if(prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._012345678")!=std::string::npos)
        {
            // 如果前缀不合法，则记录错误日志
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())<<"Config invalid name:"<<prefix<<":"<<node;
        }
        // 将前缀和节点添加到输出列表中
        output.push_back(std::make_pair(prefix,node));
        if(node.IsMap()){
            // 如果是映射，则遍历所有子节点
            for(auto it=node.begin();
                it !=node.end();++it){
                    ListAllMember(prefix.empty()?it->first.Scalar()
                    :prefix+ "." + it->first.Scalar(),it->second,output);
                }
        }
    }
    void Config::LoadFromYaml(const YAML::Node& root){
        std::list<std::pair<std::string,const YAML::Node>> all_nodes;
        ListAllMember("", root, all_nodes);
        for(auto&i :all_nodes){
            std::string key = i.first;
            if (key.empty()){
                continue;
            }

            std::transform(key.begin(),key.end(),key.begin(),::tolower);
            ConfigVarBase::ptr var =LookupBase(key);
            if(var){
                if(i.second.IsScalar()){
                    var->fromString(i.second.Scalar());
                }
                else{
                    std::stringstream ss;
                    ss<<i.second;
                    var->fromString(ss.str());
                }
            }
        }
    }
}
