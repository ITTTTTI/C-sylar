#include "../sylar/sylar.h"
#include <assert.h>

sylar::Logger::ptr Logger = SYLAR_LOG_ROOT();

void test_assert(){
    SYLAR_LOG_INFO(Logger)<<sylar::BacktraceToString(10);
    SYLAR_ASSERT2(false,"abcdef xx");

}
int main(int argc , char** argv){
    test_assert();
    return 0;
}