#include "util.h"
#include <execinfo.h>
#include <string>
#include "log.h"

#include "fiber.h"


namespace sylar
{
sylar::Logger::ptr g_logger =SYLAR_LOG_NAME("system");
pid_t GetThreadId() {
    return syscall(SYS_gettid);
}
uint32_t GetFiberId() {
    return sylar::Fiber::GetFiberId();
}
void Backtrace(std::vector<std::string>& bt , int size, int skip){
    void** array =(void**) malloc(sizeof(void*)*size);
    size_t s =::backtrace(array,size);//调用backtrace函数（注意前面的::表示这是一个全局函数，而非某个类的成员函数）来填充array数组。

    char** strings = backtrace_symbols(array, s);
    if (strings==NULL){
        SYLAR_LOG_ERROR(g_logger)<<"backtrace_synbols error";
        return;
    }

    for(size_t i =skip; i<s ;++i){
        bt.push_back(strings[i]);
    }
    free(strings);
    free(array);
}
std::string BacktraceToString(int size,int skip,const std::string& prefix ){
    std::vector<std::string> bt;
    Backtrace(bt,size,skip);
    std::stringstream ss;
    for(size_t i=0;i<bt.size();++i)//size_t 是一种无符号整数类型，它用于表示大小、索引和计数等。
    {
        ss<<prefix<<bt[i]<<std::endl;
    }
    return ss.str();


}
} // namespace name
