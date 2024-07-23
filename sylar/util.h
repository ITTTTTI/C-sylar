#ifndef __SYLAR_UTIL_H__
#define __SYLAR_UTIL_H__

#include <pthread.h>
#include <sys/types.h>
#include<sys/syscall.h>
#include <stdio.h>
#include <unistd.h>
#include <cstdint>

namespace sylar {
uint32_t GetFiberId();
pid_t GetThreadId();
}

#endif