#include "../sylar/sylar.h"
#include <iostream>

static sylar::Logger::ptr g_logger = SYLAR_LOG_ROOT();

void test_fiber(){
    SYLAR_LOG_INFO(g_logger)<<"test in fiber";

    static int s_count =3;
    sleep(1);
    if(--s_count>=0){
        sylar::Scheduler::GetThis()->schedule(&test_fiber);
    }
}
int main(int argc, char** argv)
{
    SYLAR_LOG_INFO(g_logger)<<"main";
    sylar::Scheduler sc;
    sc.start();
    sc.schedule(test_fiber);
    sc.stop();
    SYLAR_LOG_INFO(g_logger)<<"over";
    return 0;
}