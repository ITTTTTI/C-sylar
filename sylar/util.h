#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <pthread.h>
#include <sys/types.h>
#include<sys/syscall.h>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <cstdint>
#include <vector>

namespace sylar {
uint32_t GetFiberId();
pid_t GetThreadId();
void Backtrace(std::vector<std::string>& bt , int size, int skip);
std::string BacktraceToString(int size=64,int skip=1,const std::string& prefix="");

}

#endif