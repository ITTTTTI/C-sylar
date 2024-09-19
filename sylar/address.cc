#include "address.h"
#include <sstream>
#include <string.h>
#include <cstring>
#include "endian.h"
#include "log.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <ifaddrs.h>


static sylar::Logger::ptr g_logger = SYLAR_LOG_NAME("system");

namespace sylar{

template<class T>
static T CreateMask(uint32_t bits){
    return (1<<(sizeof(T)*8-bits))-1;
}

template<class T>
static uint32_t CountBytes(T value){
    uint32_t result=0;
    for(;value;++result){
        value &=value-1;
    }
    return result;

}

Address::ptr Address::LookupAny(const std::string& host,
                       int family,int type, int protocol){
        std::vector<Address::ptr> result;
        if(Lookup(result,host,family,type,protocol)){
            return result[0];
        }
        return nullptr;
                       }
std::shared_ptr<IPAddress> Address::LookupAnyIPAddress(const std::string& host,
                       int family,int type, int protocol){
        std::vector<Address::ptr> result;
        if(Lookup(result,host,family,type,protocol)){
            for(auto& i:result){
            IPAddress::ptr v=std::dynamic_pointer_cast<IPAddress>(i);
            if(v)
            {
                return v;
            }
        }
        }
        return nullptr;
    }



bool Address::Lookup(std::vector<Address::ptr>& result,const std::string& host,
                       int family,int type, int protocol){
        addrinfo hints,*results,*next;//地址解析
        memset(&hints, 0, sizeof(hints));
        hints.ai_flags=0;
        hints.ai_family=family;
        hints.ai_socktype=type;
        hints.ai_protocol=protocol;
        hints.ai_canonname=NULL;
        hints.ai_addr=NULL;
        hints.ai_next=NULL;//ai_next是一个指针，用于指向下一个addrinfo结构体，从而形成一个链表结构。

        std::string node;
        const char* service =NULL;
        

        //检查ipv6地址
        if(!host.empty()&& host[0] == '['){
            const char* endipv6=(const char*)memchr(host.c_str()+1,']',host.size()-1);
            if(endipv6){
                if(*(endipv6+1)==':'){
                    service =endipv6 +2;
                }
                node = host.substr(1,endipv6-host.c_str()-1);
        }
        }
        
        //检查 node service
        if(node.empty())
        {
            service =(const char*)memchr(host.c_str()+1,']',host.size());
            if(service){
                if(!memchr(service+1,':',host.c_str()+host.size()-service-1)){
                    node=host.substr(0,service-host.c_str());
                    ++service;

                }
            }
        }

        

        if(node.empty()){
            node=host;
        }

        int error=getaddrinfo(node.c_str(),service, &hints,&results);

        //将主机名或服务名转换成相应的套接字地址
        if(error){
            SYLAR_LOG_ERROR(g_logger)<<"Address::Lookup error: "
            <<host<<","<<family;
            return false;
        }


        next=results;
        while(next){
            result.push_back(Create(next->ai_addr,(socklen_t)next->ai_addrlen));
            next=next->ai_next;
        }

        freeaddrinfo(results);
        return !result.empty();

}


bool Address::GetInterfaceAddresses(std::multimap<std::string,std::pair<Address::ptr,uint32_t>>& result,
                     int family){                 
    struct ifaddrs *next, *results;
    
    if(getifaddrs(&results)!=0){
        SYLAR_LOG_ERROR(g_logger) <<"Address::GetInterfaceAddresses error="
            <<errno<<"errstr=" <<strerror(errno);
            return false;
            }
    try{
        for(next=results;next;next=next->ifa_next){
            Address::ptr addr;
            uint32_t prefix_len=~0u;
            if(family!=AF_UNSPEC&& family != next->ifa_addr->sa_family){
                continue;
            }
            switch(next->ifa_addr->sa_family){
                case AF_INET:
                    {
                        addr=Create(next->ifa_addr,sizeof(sockaddr_in));
                        uint32_t netmask=((sockaddr_in *)next->ifa_netmask)->sin_addr.s_addr;
                        prefix_len=CountBytes(netmask);
                    }
                    break;
                case AF_INET6:
                    {
                        addr=Create(next->ifa_addr,sizeof(sockaddr_in6));
                        in6_addr& netmask=((sockaddr_in6 *)next->ifa_netmask)->sin6_addr;
                        prefix_len=0;
                        for(int i=0;i<16;++i){
                            prefix_len+=CountBytes(netmask.s6_addr[i]);
                        }
                    }
                    break;
                default:
                    break;
                }
                if(addr){
                    result.insert(std::make_pair(next->ifa_name,
                    std::make_pair(addr,prefix_len)));
                }
           }
        }catch(...){
                SYLAR_LOG_ERROR(g_logger)<<"Address::GetInterfaceAddresses exception";
                freeifaddrs(results);
                return false;
            }
       freeifaddrs(results);
        return true; 
}


bool Address::GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >&result,const std::string& iface, int family){
         if(iface.empty()||iface=="*"){
            if(family==AF_INET || family==AF_UNSPEC) {
                result.push_back(std::make_pair(Address::ptr(new IPv4Address()),0u));
            }
            if(family==AF_INET6||family==AF_UNSPEC){
                result.push_back(std::make_pair(Address::ptr(new IPv6Address()),0u));
            }
            return true;
         } 
         std::multimap<std::string
         , std::pair<Address::ptr, uint32_t> > results;

         if(!GetInterfaceAddresses(results,family)){
             return false;
         }

        auto its =results.equal_range(iface);
        //这个函数返回一个包含两个迭代器的 pair，
        //这两个迭代器分别指向容器中所有与给定键值相等的元素的首个元素和最后一个元素之后的位置。
         for(;its.first != its.second;++its.first){
            result.push_back(its.first->second);
         }

         return true;


}

int Address::getFamily() const {
    return getAddr()->sa_family;
 }



std::string Address::toString(){
    std::stringstream ss;
    insert(ss);
    return ss.str();
    
}

Address::ptr Address::Create(const sockaddr* addr, socklen_t addrlen){
        if(addr==nullptr)
          return nullptr;

        Address::ptr result;
        switch(addr->sa_family){
            case AF_INET:
                 result.reset(new IPv4Address(*(const sockaddr_in*)addr));
                break;
            case AF_INET6:
                result.reset(new IPv6Address(*(const sockaddr_in6*)addr));
                break;
            default:
                result.reset(new UnknownAddress(*addr));
                break;

        }
        return result;
}

bool Address::operator<(const Address &rhs) const{
    socklen_t minlen=std::min(getAddrLen(),rhs.getAddrLen());
    int result =memcmp(getAddr(),rhs.getAddr(),minlen);
    //逐字节比较两块内存区域，直到发现不相等的字节或达到指定的比较长度。
    if(result<0){
        return true;
    }
    else if(result >0){
        return false;
    }else if(getAddrLen()<rhs.getAddrLen())
   {
    return true;
   }
   return false;
}

bool Address::operator==(const Address &rhs) const{
    return getAddrLen()==rhs.getAddrLen()
    &&memcmp(getAddr(), rhs.getAddr(), getAddrLen())==0;
}

bool Address::operator!=(const Address &rhs) const{
    return !(*this==rhs);

}

IPAddress::ptr IPAddress::Create(const char* address, uint16_t port){
    addrinfo hints, *results;
    memset(&hints, 0, sizeof(hints));

    hints.ai_flags=AI_NUMERICSERV;
    hints.ai_family=AF_UNSPEC;

    int error =getaddrinfo(address, NULL, &hints, &results);
    if(error){
        SYLAR_LOG_ERROR(g_logger)<<"IPvAddress::Create ("<<address<<","
        <<port<<") rt="<<port<<" errno="<<errno
        <<" errstr="<<strerror(errno);
        return nullptr;
    } 

    try{
        IPAddress::ptr result = std::dynamic_pointer_cast<IPAddress>(
        Address::Create(results->ai_addr, (socklen_t)results->ai_addrlen));
        if(result){
            result->setPort(port);
        }
        freeaddrinfo(results);
        return result;
    } catch(...){
            freeaddrinfo(results);
            return nullptr;
    }
}

IPv4Address::ptr IPv4Address::Create(const char* address, uint16_t port ){
    IPv4Address::ptr rt(new IPv4Address);
    rt->m_addr.sin_port=byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET, address, &rt->m_addr.sin_addr);
    if(result <= 0)
    {
        SYLAR_LOG_ERROR(g_logger)<<"IPv4Address::Create ("<<address<<","
        <<port<<") rt="<<port<<" errno="<<errno
        <<" errstr="<<strerror(errno);
        return nullptr;
    }
    return rt;
}

IPv4Address::IPv4Address(const sockaddr_in& address){
    m_addr=address;
}

IPv4Address:: IPv4Address(uint32_t address , uint16_t port){
    memset(&m_addr, 0,sizeof(m_addr));//初始化
    /*struct sockaddr_in {
    short int          sin_family;  // 地址族 (AF_INET)
    unsigned short int sin_port;    // 端口号
    struct in_addr     sin_addr;    // IP 地址
    unsigned char      sin_zero[8]; // 填充
    };*/
    m_addr.sin_family=AF_INET;//IPv4
    m_addr.sin_port=byteswapOnLittleEndian(port); 
    /*网络字节序
    在网络通信中，为了保持数据的一致性和可移植性，
    所有参与通信的设备都应当遵循相同的字节序来传输数据。
    计算机体系结构（如x86, x64）使用小端字节序（Little-Endian）
    来表示多字节数据，因此在发送数据之前，
    需要将数据从主机的字节序转换为网络字节序，并在接收端进行相应的转换*/
    m_addr.sin_addr.s_addr = byteswapOnLittleEndian(address);
}

const sockaddr* IPv4Address:: getAddr() const {
    return (sockaddr*)&m_addr;
}

socklen_t IPv4Address:: getAddrLen() const {
    return sizeof(m_addr);
}

std::ostream& IPv4Address:: insert(std::ostream &os) const {
    uint32_t addr = byteswapOnLittleEndian(m_addr.sin_addr.s_addr);
    os<<((addr>>24)&0xff)<<"."
    <<((addr>>16)&0xff)<<"."
    <<((addr>>8)&0xff)<<"."
    <<(addr&0xff);

    os<<":"<<byteswapOnLittleEndian(m_addr.sin_port);
    return os;
}
IPAddress::ptr IPv4Address:: broadcastAddress(uint32_t prefix_len) const {
    if(prefix_len >32 ){
        return nullptr;
    }

    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr |= 
    byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    
    return IPv4Address::ptr(new IPv4Address(baddr));
    
    
}
IPAddress::ptr IPv4Address:: networdAress(uint32_t prefix_len) const {
     if(prefix_len >32 ){
        return nullptr;
    }

    sockaddr_in baddr(m_addr);
    baddr.sin_addr.s_addr &= 
    byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    
    return IPv4Address::ptr(new IPv4Address(baddr));
    
}
IPAddress::ptr IPv4Address:: subnetMask(uint32_t prefix_len) const {
    sockaddr_in subnet;
    memset(&subnet, 0, sizeof(sockaddr_in));
    subnet.sin_family=AF_INET;
    subnet.sin_addr.s_addr = 
    byteswapOnLittleEndian(CreateMask<uint32_t>(prefix_len));
    return IPv4Address::ptr(new IPv4Address(subnet));
}
uint32_t IPv4Address:: getPort() const {
    return byteswapOnLittleEndian(m_addr.sin_port);
    
}
void IPv4Address:: setPort(uint16_t v){
    m_addr.sin_port=byteswapOnLittleEndian(v);
}

IPv6Address::ptr IPv6Address::Create(const char* address, uint16_t port){
    IPv6Address::ptr rt(new IPv6Address);
    rt->m_addr.sin6_port = byteswapOnLittleEndian(port);
    int result = inet_pton(AF_INET6, address, &rt->m_addr.sin6_addr);
    if(result <= 0)
    {
        SYLAR_LOG_ERROR(g_logger)<<"IPv6Address::Create ("<<address<<","
        <<port<<") rt="<<result<<" errno="<<errno
        <<" errstr="<<strerror(errno);
        return nullptr;
    }
    return rt;
}

 IPv6Address::IPv6Address(const sockaddr_in6& address){
    m_addr=address;
 }

IPv6Address::IPv6Address(){
    memset(&m_addr, 0,sizeof(m_addr));//初始化
    m_addr.sin6_family=AF_INET6;
    m_addr.sin6_port=0;
}

IPv6Address::IPv6Address(const uint8_t address[16],uint16_t port){
    memset(&m_addr, 0,sizeof(m_addr));//初始化
    m_addr.sin6_family=AF_INET6;
    m_addr.sin6_port=byteswapOnLittleEndian(port); 
    memcpy(&m_addr.sin6_addr.s6_addr,address, 16);
}
const sockaddr* IPv6Address:: getAddr() const {
    return (sockaddr*)&m_addr;
}



socklen_t IPv6Address:: getAddrLen() const {
    return sizeof(m_addr);
    
}
std::ostream& IPv6Address:: insert(std::ostream &os) const {
    os <<"[";
    uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
    bool used_zero = false;
    for(size_t i=0;i<8;++i){
        if(addr[i]==0&&!used_zero){
            continue;
        }
        if(i&&addr[i-1]==0&&!used_zero){
            os<<":";
            used_zero=true;
        }
        if(i){
            os<<":";
        }
        os<<std::hex<<(int)byteswapOnLittleEndian(addr[i])<<std::dec;
    }
    if(used_zero && addr[7] == 0)
            os<<"::";
    
    os<< "]:" << byteswapOnLittleEndian(m_addr.sin6_port);
    return os;
    
}
IPAddress::ptr IPv6Address:: broadcastAddress(uint32_t prefix_len) const {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len/8] |= 
    CreateMask<uint8_t>(prefix_len%8);
    for(int i = prefix_len/8 +1;i<16;i++)
    {
        baddr.sin6_addr.s6_addr[i] = 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(baddr));
}
IPAddress::ptr IPv6Address:: networdAress(uint32_t prefix_len) const {
    sockaddr_in6 baddr(m_addr);
    baddr.sin6_addr.s6_addr[prefix_len/8] &= 
    CreateMask<uint8_t>(prefix_len%8);
    // for(int i = prefix_len/8 +1;i<16;i++)
    // {
    //     baddr.sin6_addr.s6_addr[i] = 0xff;
    // }
    return IPv6Address::ptr(new IPv6Address(baddr));
    
}
IPAddress::ptr IPv6Address:: subnetMask(uint32_t prefix_len) const {
    sockaddr_in6 subnet(m_addr);
    memset(&subnet, 0, sizeof(sockaddr_in6));
    subnet.sin6_family=AF_INET6;
    subnet.sin6_addr.s6_addr[prefix_len/8] = 
    ~CreateMask<uint8_t>(prefix_len%8);

    for(int i = prefix_len/8 +1;i<8;i++){
        subnet.sin6_addr.s6_addr[i]= 0xff;
    }
    return IPv6Address::ptr(new IPv6Address(subnet));

    
}
uint32_t IPv6Address:: getPort() const {
    return byteswapOnLittleEndian(m_addr.sin6_port);
}
void IPv6Address:: setPort(uint16_t v){
    m_addr.sin6_port=byteswapOnLittleEndian(v);
}

static const size_t MAX_PATH_LEN =sizeof(((sockaddr_un*)0)->sun_path)-1;
UnixAddress::UnixAddress(){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family=AF_UNIX;
    m_length=offsetof(sockaddr_un,sun_path)+ MAX_PATH_LEN;
}

UnixAddress::UnixAddress(const std::string& path){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sun_family=AF_UNIX;
    m_length=path.size()+1;

   if(!path.empty() && path[0]=='\0'){
        --m_length;
   }

   if(m_length>sizeof(m_addr.sun_path)){
      throw std::logic_error("path too long");
   }

   memcpy(m_addr.sun_path,path.c_str(),m_length);
   //c_str()成员函数返回一个指向以空字符终止的数组的指针，该数组包含了与std::string对象相同的字符序列。
   m_length+=offsetof(sockaddr_un,sun_path);
    //获取结构体中某个成员相对于结构体开头的偏移量（以字节为单位
}

const sockaddr* UnixAddress::getAddr() const{
       return (sockaddr*)&m_addr;
}
socklen_t UnixAddress::getAddrLen() const {
    return m_length;
    
}
std::ostream& UnixAddress::insert(std::ostream &os) const {
    if(m_length>offsetof(sockaddr_un,sun_path)&& m_addr.sun_path[0]=='\0')
    {
        return os<<"\\0"<<std::string(m_addr.sun_path+1,
             m_length-offsetof(sockaddr_un,sun_path)-1);
    }
    return os<<m_addr.sun_path;
        
}



UnknownAddress::UnknownAddress(int family) {
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sa_family = family;
}

UnknownAddress::UnknownAddress(const sockaddr& addr) {
    m_addr = addr;
}
const sockaddr* UnknownAddress::getAddr() const {
    return &m_addr;
}


socklen_t UnknownAddress::getAddrLen() const {
    return sizeof(m_addr);

}

std::ostream& UnknownAddress::insert(std::ostream &os) const {
       os<<"[Unknownaddress family="<<m_addr.sa_family<<"]";
       return os;
}

}