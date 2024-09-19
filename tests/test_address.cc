#include "../sylar/sylar.h"
#include <map>
#include <vector>
#include <iostream>


sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test(){
    std::vector<sylar::Address::ptr> addrs;

    bool v =sylar::Address::Lookup(addrs,"www.baidu.com");
    if(!v){
        SYLAR_LOG_ERROR(g_logger) << "lookup fail";
    }
    
    for(size_t i = 0;i < addrs.size();++i){
        SYLAR_LOG_INFO(g_logger) << i<<"-"<<addrs[i]->toString();
    }
}


void test_iface(){
    std::multimap<std::string,std::pair<sylar::Address::ptr, uint32_t>> results;

    bool v=sylar::Address::GetInterfaceAddresses(results);
    if(!v){
        SYLAR_LOG_ERROR(g_logger)<<"get interface address fail";
    }
    for(auto& i : results){
        SYLAR_LOG_INFO(g_logger)<<i.first<<" - "
        <<i.second.first->toString()<<" - "<<i.second.second;
    }
}


void test_ipv4(){
    auto addr=sylar::IPAddress::Create("baidu.com");
    if(addr){
        SYLAR_LOG_INFO(g_logger)<<addr->toString();
    }
}

int main(int argc, char** argv){
    //test();
    //test_iface();
    test_ipv4();
    return 0;
}