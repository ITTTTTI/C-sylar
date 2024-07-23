#include "log.h"
#include <map>
#include <string>
#include <iostream>
#include <tuple>
#include <vector>
#include <functional>



namespace sylar 
{

const char* LogLevel::ToString(LogLevel::Level level){
    switch (level)
    {
#define XX(name) \
    case LogLevel::name: \
    return #name; \
        break;
    XX(DEBUG);
    XX(INFO);
    XX(WARN);
    XX(ERROR);
    XX(FATAL);
#undef XX
    default:
        return "UNKNOWN";
    }
    return "UNKNOWN";

}

LogEventWrap::LogEventWrap(LogEvent::ptr e):m_event(e){

}

LogEventWrap::~LogEventWrap(){
    m_event->getLogger()->log(m_event->getLevel(),m_event);
}

std::stringstream& LogEventWrap::getSS(){
       return m_event->getSS();
}
class MessageFormatItem:public LogFormatter::FormatItem{
public:
    MessageFormatItem(const std::string& str=""){ }
    void format(std::ostream& os,std::shared_ptr<Logger> logger, LogLevel::Level Level,LogEvent::ptr event) override{
        os<<event->getContent();
    }


    };

class LevelFormatItem:public LogFormatter::FormatItem{
public:
    LevelFormatItem(const std::string& str=""){ }
    void format(std::ostream& os, Logger::ptr logger , LogLevel::Level level,LogEvent::ptr event) override{
        os<<LogLevel::ToString(level);
    };
   
};


class NameFormatItem:public LogFormatter::FormatItem{
public:
    NameFormatItem(const std::string& str=""){ }
    void format(std::ostream& os, Logger::ptr logger , LogLevel::Level level,LogEvent::ptr event) override{
        os<<logger->getName();
};
};

class ElapseFormatItem:public LogFormatter::FormatItem{
public:
    ElapseFormatItem(const std::string& str=""){ }
    void format(std::ostream& os, Logger::ptr logger , LogLevel::Level level,LogEvent::ptr event) override{
        os<<event->getElapse();
};
};

class ThreadIdFormatItem:public LogFormatter::FormatItem{
public:
    ThreadIdFormatItem(const std::string& str=""){ }
    void format(std::ostream& os, Logger::ptr logger , LogLevel::Level level,LogEvent::ptr event) override{
        os<<event->getThreadId();
};
};

class FiberIdFormatItem:public LogFormatter::FormatItem{
public:
    FiberIdFormatItem(const std::string& str=""){ }
        void format(std::ostream& os, Logger::ptr logger , LogLevel::Level level,LogEvent::ptr event) override{
            // 输出事件所属的Fiber ID
            os<<event->getFiberId();


    };
};

class DateTimeFormatItem:public LogFormatter::FormatItem{
public:
        DateTimeFormatItem(const std::string& format ="%Y:%m:%d %H:%M:%S") // 构造函数，接受一个可选的日期时间格式字符串作为参数
            :m_format(format) // 初始化成员变量m_format为传入的参数值
        {
            // 如果m_format为空
            if(m_format.empty()){
                // 将m_format设置为默认的日期时间格式
                m_format = "%Y:%m:%d %H:%M:%S";
            }
        }
        void format(std::ostream& os,Logger::ptr logger , LogLevel::Level level,LogEvent::ptr event) override{
            // 定义一个time_t类型的结构体tm，用于存储本地时间
            struct tm tm;
            // 获取事件的时间戳
            time_t time =event->getTime();
            // 将时间戳转换为本地时间，并存储在tm结构体中
            localtime_r( &time, &tm);
            // 定义一个字符数组buf，用于存储格式化后的时间字符串
            char buf[64];
            // 使用strftime函数将tm结构体中的时间按照指定的格式m_format转换为字符串，并存储在buf中
            strftime(buf, sizeof buf, m_format.c_str(), &tm);
            // 将格式化后的时间字符串输出到ostream中
            os<<buf;
        }
private:
    std::string m_format;
};


class FileNameFormatItem:public LogFormatter::FormatItem{
public:
        FileNameFormatItem(const std::string& str=""){ }
        void format(std::ostream& os, Logger::ptr logger , LogLevel::Level level,LogEvent::ptr event) override{
            // 输出事件所属的Fiber ID
            os<<event->getFile();


    };
};

class LineFormatItem:public LogFormatter::FormatItem{
public:
        LineFormatItem(const std::string& str=""){ }//构造函数不用分号
            void format(std::ostream& os, Logger::ptr logger , LogLevel::Level level,LogEvent::ptr event) override{
                // 输出事件所属的Fiber ID
                // 输出事件所在的行号
                // 输出事件所属的Fiber ID
                os << event->getLine();


        };
};

class NewLineFormatItem:public LogFormatter::FormatItem{
public:
        NewLineFormatItem(const std::string& str=""){ }
        void format(std::ostream& os, Logger::ptr logger , LogLevel::Level level,LogEvent::ptr event) override{
            // 输出事件所属的Fiber ID
            os<<std::endl;


    };
};

class StringFormatItem:public LogFormatter::FormatItem{
public:
        StringFormatItem(const std::string& str): m_string(str){}
        void format(std::ostream& os, Logger::ptr logger , LogLevel::Level level,LogEvent::ptr event) override{
            // 输出事件所属的Fiber ID
            os<<m_string;
        }

private:
    std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem {
    public:
    TabFormatItem(const std::string& str=""){}
    void format(std::ostream& os, Logger::ptr logger , LogLevel::Level level,LogEvent::ptr event) override
    {
        os<<"\t";
    }
 private:
      std::string m_string;
};

LogEvent::LogEvent(std::shared_ptr<Logger> logger,LogLevel::Level level,
const char* file,int32_t line,uint32_t elapse, uint32_t threadId, uint32_t fiberId, uint64_t time)
:m_file(file),
m_line(line),
m_elapse(elapse),
m_threadId(threadId),
m_fiberId(fiberId),
m_time(time),
m_logger(logger),
m_level(level)
{
}

Logger::Logger(const std::string& name):m_name(name),m_level(LogLevel::DEBUG) {
            m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    }

void Logger::addAppender(LogAppender::ptr appender){
    // 调用成员函数addAppender添加一个appender
    if(!appender->getFormatter()){
        appender->setFormatter(m_formatter);
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr delAppender){
    // 遍历日志记录器中的日志附加器列表
    for (auto it= m_appenders.begin(); it != m_appenders.end(); ++it){
        // 如果找到与要删除的附加器相等的附加器
        if (*it == delAppender) {
            // 则从列表中删除该附加器
            m_appenders.erase(it);
            // 跳出循环
            break;
        }
    }
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event){
    // 如果日志级别大于等于当前日志级别
    if(level>=m_level){
        auto self =shared_from_this();
        // 遍历所有的日志输出器
        for(auto&i : m_appenders){
            // 调用日志输出器的log方法，记录日志
            i->log(self,level, event);
        }
    }
}

void Logger::debug(LogEvent::ptr event){
    // 调用log方法，记录DEBUG级别的日志
    log(LogLevel::DEBUG, event);
}
    
void Logger::info(LogEvent::ptr event){
    // 调用log方法，记录INFO级别的日志
    log(LogLevel::INFO, event);

}

void Logger::warn(LogEvent::ptr event){
    // 调用log方法，记录WARN级别的日志
    log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::ptr event){
    // 调用log方法，记录ERROR级别的日志
    log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::ptr event){
    // 调用log方法，记录FATAL级别的日志
    log(LogLevel::FATAL, event);
}



FileLogAppender::FileLogAppender(const std::string& filename):m_filename(filename)
{

}
    
void FileLogAppender::log(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)
{
    // 判断日志级别是否大于或等于当前设置的日志级别
    if(level >= m_level){
        // 使用格式化器对日志事件进行格式化，并将结果写入文件流
        m_filestream<<m_formatter->format(logger,level,event);
    }

}

bool FileLogAppender::reopen(){
     if(m_filestream){
        m_filestream.close();
     }
     m_filestream.open(m_filename);
     //!! 是一个常见的双重逻辑非操作符。这个操作符首先对其操作数应用逻辑非 (!)，然后再对结果应用逻辑非。这样做的一个常见原因是为了将某个值转换为标准的布尔值 true 或 false。
     return !!m_filestream;
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event){
     // 如果传入的日志级别大于或等于当前日志级别
     if(level>=m_level){
        // 使用格式化器对事件进行格式化，并将结果输出到标准输出流
        std::cout<<m_formatter->format(logger,level,event);
     }
}
LogFormatter::LogFormatter(const std::string& pattern):m_pattern(pattern){

}
void LogEvent::format(const char* fmt,...){
    // 定义一个va_list类型的变量al，用于存储变长参数列表
    va_list al;
    // 初始化al，将fmt之后的参数列表的地址传给al
    va_start(al,fmt);
    // 调用另一个format函数，传入格式化字符串fmt和参数列表al
    format(fmt,al);
    // 清理al所占用的内存
    va_end(al);
}
void LogEvent::format(const char* fmt, va_list al){
            // 定义一个字符指针buf，初始化为nullptr
            char* buf =nullptr;
            // 使用vasprintf函数将格式化字符串fmt和参数列表al转化为字符串，并返回字符串长度len，同时将字符串的地址保存在buf中
            int len= vasprintf(&buf,fmt,al);
            // 如果len不等于-1，表示字符串转换成功
            if(len!=-1){
                // 将buf指向的字符串转换为std::string类型，并添加到m_ss字符串流中
                m_ss<<std::string(buf,len);
                // 释放buf指向的内存空间
                free(buf);
            }

}
std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level,LogEvent::ptr event){
  std::stringstream ss;
  // 遍历 m_items 中的每个元素
  for(auto& i:m_items){
    // 调用当前元素的 format 方法，将结果输出到字符串流 ss 中，并传入 event 参数
    i->format(ss,logger,level,event);
  }
  // 返回字符串流 ss 中的字符串
  return ss.str();
}

void LogFormatter::init() {
    //str, format, type
    std::vector<std::tuple<std::string, std::string, int> > vec;
    std::string nstr;
    for(size_t i = 0; i < m_pattern.size(); ++i) {
        if(m_pattern[i] != '%') {
            nstr.append(1, m_pattern[i]);
            continue;
        }

        if((i + 1) < m_pattern.size()) {
            if(m_pattern[i + 1] == '%') {
                nstr.append(1, '%');
                continue;
            }
        }

        size_t n = i + 1;
        int fmt_status = 0;
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;
        while(n < m_pattern.size()) {
            if (!isalpha(m_pattern[n])&& m_pattern[n!='{']){
                break;
                }
            if(fmt_status == 0) {
                if(m_pattern[n] == '{') {
                    str = m_pattern.substr(i + 1, n - i - 1);
                    //std::cout << "*" << str << std::endl;
                    fmt_status = 1; //解析格式
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            } 
             if(fmt_status == 1) {
                if(m_pattern[n] == '}') {
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    //std::cout << "#" << fmt << std::endl;
                    fmt_status = 2;
                    break;
                }
            }
            ++n;
            }
            
        

        if(fmt_status == 0) {
            if(!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            str = m_pattern.substr(i + 1, n - i - 1);
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n ;
        } else if(fmt_status == 1) {
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }else if(fmt_status==2){
            if(!nstr.empty()){
                vec.push_back(std::make_tuple(nstr,"",0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str,fmt,1));
            i=n;
        }
    }

    if(!nstr.empty()) {
        vec.push_back(std::make_tuple(nstr, "", 0));
    }
    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)> > s_format_items = {
#define XX(str, C) \
        {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));}}

        XX(m, MessageFormatItem),           //m:消息
        XX(p, LevelFormatItem),             //p:日志级别
        XX(r, ElapseFormatItem),            //r:累计毫秒数
        XX(c, NameFormatItem),              //c:日志名称
        XX(t, ThreadIdFormatItem),          //t:线程id
        XX(n, NewLineFormatItem),           //n:换行
        XX(d, DateTimeFormatItem),          //d:时间
        XX(f, FileNameFormatItem),          //f:文件名
        XX(l, LineFormatItem),              //l:行号
        XX(T, TabFormatItem),               //T:Tab
        XX(F, FiberIdFormatItem),           //F:协程id
#undef XX
    };

    for(auto& i : vec) {
        if(std::get<2>(i) == 0) {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } else {
            auto it = s_format_items.find(std::get<0>(i));
            if(it == s_format_items.end()) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
            } else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }

        std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
    }
    std::cout << m_items.size() << std::endl;
}




LoggerManager::LoggerManager(){
       m_root.reset(new Logger);

       m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
}

Logger::ptr LoggerManager::getLogger(const std::string& name){
        auto it =m_loggers.find(name);
        return it ==m_loggers.end()?m_root:it->second;
        
}


 /**%m--消息体
         * %p--输出优先级level
         * %r--启动后的时间
         * %c--日志名称
         * %t--线程id
         * %n--回车换行
         * %d--日期
         * %f--文件名
         * %l--行号
         */
}




