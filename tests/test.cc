#include<iostream>
#include"../sylar/log.h"
#include"../sylar/util.h"
#include<thread>
 
int main(int argc,char** argv){
    sylar::Logger::ptr logger(new sylar::Logger);
    logger->addAppender(sylar::LogAppender::ptr(new sylar::StdoutLogAppender));
    sylar::FileLogAppender::ptr file_appender(new sylar::FileLogAppender("./test.log"));
    sylar::LogFormatter::ptr fmt(new sylar::LogFormatter("%d%T%p%T%m%n"));
    file_appender->setFormatter(fmt);
    file_appender->setLevel(sylar::LogLevel::ERROR);
    logger->addAppender(file_appender);

    // sylar::LogEvent::ptr event(new sylar::LogEvent(__FILE__,__LINE__,0,sylar::GetThreadId(),sylar::GetFiberId(),time(0)));
    // event->getSS()<<"hello sylar log";
    // logger->log(sylar::LogLevel::DEBUG,event);
    std::cout<<"Hello World"<<std::endl;
    SYLAR_LOG_INFO(logger)<<"test macro";
    SYLAR_LOG_ERROR(logger)<<"test macro";

    SYLAR_LOG_FMT_ERROR(logger,"test macro fmt error %s","aa");

    auto l= sylar::LoggerMgr::GetInstance()->getLogger("xx");

    SYLAR_LOG_INFO(l)<<"xxx";

    return 0;
}