#include "log.h"
#include <map>
#include <string>
#include <iostream>
#include <tuple>
#include <vector>
#include <functional>
#include <time.h>
#include "config.h"
#include <iostream>
#include "thread.h"



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

LogLevel::Level LogLevel::FromString(const std::string& str){
#define XX(level,v)    \
    if(str== #v){\
        return LogLevel::level; \
    }

    XX(DEBUG,debug);
    XX(INFO,info);
    XX(WARN,warn);
    XX(ERROR,error);
    XX(FATAL,fatal);

    XX(DEBUG,DEBUG);
    XX(INFO,INFO);
    XX(WARN,WARN);
    XX(ERROR,ERROR);
    XX(FATAL,FATAL);
    return LogLevel::UNKONW;
#undef XX
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
}
   
};


class NameFormatItem:public LogFormatter::FormatItem{
public:
NameFormatItem(const std::string& str=""){ }
void format(std::ostream& os, Logger::ptr logger , LogLevel::Level level,LogEvent::ptr event) override{
    os<<event->getLogger()->getName();
}
};

class ElapseFormatItem:public LogFormatter::FormatItem{
public:
ElapseFormatItem(const std::string& str=""){ }
void format(std::ostream& os, Logger::ptr logger , LogLevel::Level level,LogEvent::ptr event) override{
    os<<event->getElapse();
}
};

class ThreadIdFormatItem:public LogFormatter::FormatItem{
public:
ThreadIdFormatItem(const std::string& str=""){ }
void format(std::ostream& os, Logger::ptr logger , LogLevel::Level level,LogEvent::ptr event) override{
    os<<event->getThreadId();
}
};

class FiberIdFormatItem:public LogFormatter::FormatItem{
public:
FiberIdFormatItem(const std::string& str=""){ }
void format(std::ostream& os, Logger::ptr logger , LogLevel::Level level,LogEvent::ptr event) override{
    // 输出事件所属的Fiber ID
    os<<event->getFiberId();
}
};

class ThreadNameFormatItem:public LogFormatter::FormatItem{
public:
ThreadNameFormatItem(const std::string& str=""){ }
void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level Level,LogEvent::ptr event){
    os<<event->getThreadName();
}
};

class DateTimeFormatItem:public LogFormatter::FormatItem{
public:
DateTimeFormatItem(const std::string& format ="%Y-%m-%d %H:%M:%S") // 构造函数，接受一个可选的日期时间格式字符串作为参数
    :m_format(format) // 初始化成员变量m_format为传入的参数值
{
    // 如果m_format为空
    if(m_format.empty()){
        // 将m_format设置为默认的日期时间格式
        m_format = "%Y-%m-%d %H:%M:%S";
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
}
};

class LineFormatItem:public LogFormatter::FormatItem{
public:
LineFormatItem(const std::string& str=""){ }//构造函数不用分号
void format(std::ostream& os, Logger::ptr logger , LogLevel::Level level,LogEvent::ptr event) override{
    // 输出事件所属的Fiber ID
    // 输出事件所在的行号
    // 输出事件所属的Fiber ID
    os << event->getLine();
}
};

class NewLineFormatItem:public LogFormatter::FormatItem{
public:
NewLineFormatItem(const std::string& str=""){ }
void format(std::ostream& os, Logger::ptr logger , LogLevel::Level level,LogEvent::ptr event) override{
    // 输出事件所属的Fiber ID
    os<<std::endl;
}
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

LogEvent::LogEvent(std::shared_ptr<Logger> logger,LogLevel::Level level, const char* file,
    int32_t line,uint32_t elapse, uint32_t threadId, uint32_t fiberId, uint64_t time,const std::string& thread_name)
:m_file(file),
m_line(line),
m_elapse(elapse),
m_threadId(threadId),
m_fiberId(fiberId),
m_time(time),
m_logger(logger),
m_level(level),
m_threadName(thread_name)
{
}

Logger::Logger(const std::string& name):m_name(name),m_level(LogLevel::DEBUG) {
            m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
    }

void Logger::setFormatter(LogFormatter::ptr val){
      MutexType::Lock lock(m_mutex);
      m_formatter=val;
      for(auto& i:m_appenders){
        MutexType::Lock ll(i->m_mutex);
        if(!i->m_hasFormatter)
        {
            i->m_formatter=m_formatter;
        }
      }
}
void Logger::setFormatter(const std::string& val){
    sylar::LogFormatter::ptr new_val(new sylar::LogFormatter(val));
    if(new_val->isError()){
        std::cout<<"Logger setFormatter name="<<m_name<< "value="<<" invalid formatter"<< std::endl;
        return;
    }
    //m_formatter=new_val;
    Logger::setFormatter(new_val);
   
}

std::string Logger::toYamlString(){
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["name"]=m_name;
    if(m_level!=LogLevel::UNKONW)
    {
        node["level"]=LogLevel::ToString(m_level);
    }
    node["level"]=LogLevel::ToString(m_level);
    if(m_formatter){
        node["formatter"]=m_formatter->getPattern();
    }
        
    for(auto& i: m_appenders){
        node["appenders"].push_back(YAML::Load(i->toYamlString()));
    }
    std::stringstream ss;
    ss<<node;
    return ss.str();
}

LogFormatter::ptr Logger::getFormatter(){
    MutexType::Lock lock(m_mutex);
    return m_formatter;

}

void Logger::addAppender(LogAppender::ptr appender){
    MutexType::Lock lock(m_mutex);
    // 调用成员函数addAppender添加一个appender
    if(!appender->getFormatter()){
        MutexType::Lock ll(appender->m_mutex);
        appender->m_formatter=m_formatter;
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr delAppender){
    MutexType::Lock lock(m_mutex);
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

void Logger::clearAppenders(){
    MutexType::Lock lock(m_mutex);
     m_appenders.clear();
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event){
    // 如果日志级别大于等于当前日志级别
    if(level>=m_level){
        auto self =shared_from_this();
        MutexType::Lock lock(m_mutex);
        // 遍历所有的日志输出器
        if(!m_appenders.empty())
        {
        for(auto&i : m_appenders){
            // 调用日志输出器的log方法，记录日志
            i->log(self,level, event);
        }
        }
        else if(m_root){
            m_root->log(level,event);

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

void  LogAppender::setFormatter(LogFormatter::ptr val){
    MutexType::Lock lock(m_mutex);
    m_formatter=val;
    if(m_formatter){
        m_hasFormatter=true;
    }else
    {
        m_hasFormatter=false;
    }

}

LogFormatter::ptr LogAppender::getFormatter(){
    MutexType::Lock lock(m_mutex);
    return m_formatter;
 }

FileLogAppender::FileLogAppender(const std::string& filename):m_filename(filename)
{
   reopen();
}
    
void FileLogAppender::log(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event)
{
    // 判断日志级别是否大于或等于当前设置的日志级别
    if(level >= m_level){
        uint64_t now=time(0);
        if(now !=m_lastTime){
            reopen();
            m_lastTime=now;

        }
        MutexType::Lock lock(m_mutex);
        // 使用格式化器对日志事件进行格式化，并将结果写入文件流
        if(!(m_filestream<<m_formatter->format(logger,level,event))){
            std::cout<<"error"<<std::endl;
        };
    }

}

std::string FileLogAppender::toYamlString(){
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"]="FileLogAppender";
     if(m_level!=LogLevel::UNKONW)
    {
        node["level"]=LogLevel::ToString(m_level);
    }
    node["file"]=m_filename;
    if(m_hasFormatter && m_formatter){
        node["formatter"]=m_formatter->getPattern();

    }
    std::stringstream ss;
    ss<<node;
    return ss.str();

}

bool FileLogAppender::reopen(){
     MutexType::Lock lock(m_mutex);
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
        MutexType::Lock lock(m_mutex);
        // 使用格式化器对事件进行格式化，并将结果输出到标准输出流
        std::cout<<m_formatter->format(logger,level,event);
     }
}

std::string StdoutLogAppender::toYamlString(){
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"]="StdoutLogAppender";
     if(m_level!=LogLevel::UNKONW)
    {
        node["level"]=LogLevel::ToString(m_level);
    }
    if(m_hasFormatter && m_formatter) {
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss<<node;
    return ss.str();

}

LogFormatter::LogFormatter(const std::string& pattern):m_pattern(pattern){
    //std::cout<<m_pattern<<std::endl;
    init();
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
            //std::cout<< m_pattern<<std::endl;
            if (!fmt_status && !isalpha(m_pattern[n])&& m_pattern[n]!='{'&&m_pattern[n]!='}'){
                str =m_pattern.substr(i+1,n-i-1);
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
            }else if(fmt_status == 1) {
                if(m_pattern[n] == '}') {
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
                    //std::cout << "#" << fmt << std::endl;
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if(n== m_pattern.size()){
                if(str.empty()){
                    str =m_pattern.substr(i+1);
                }
            }
            }
            
        

        if(fmt_status == 0) {
            if(!nstr.empty()) {
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n-1 ;
        } else if(fmt_status == 1) {
            std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
        }else if(fmt_status==2){
            if(!nstr.empty()){
                vec.push_back(std::make_tuple(nstr,"",0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str,fmt,1));
            i=n-1;
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
        XX(N, ThreadNameFormatItem),        //F:线程名称
#undef XX
    };

    for(auto& i : vec) {
        if(std::get<2>(i) == 0) {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        } else {
            auto it = s_format_items.find(std::get<0>(i));
            if(it == s_format_items.end()) {
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                m_error =true;
            } else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }

        //std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
    }
    //std::cout << m_items.size() << std::endl;
}




LoggerManager::LoggerManager(){
       m_root.reset(new Logger);
       m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
       m_loggers[m_root->m_name]=m_root;
       init();
}
std::string LoggerManager::toYamlString() {
    MutexType::Lock lock(m_mutex);
    YAML::Node node;   
    for(auto& i:m_loggers){
        node.push_back(YAML::Load( i.second->toYamlString()));
    }
    std::stringstream ss;
    ss<<node;
    return ss.str();
}


Logger::ptr LoggerManager::getLogger(const std::string& name){
        MutexType::Lock lock(m_mutex);
        auto it =m_loggers.find(name);
        if(it !=m_loggers.end()){
                 return it->second;
        }
        Logger::ptr logger(new Logger(name));
        logger->m_root=m_root;
        m_loggers[name]=logger;
        return logger;        
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
struct LogAppenderDefine{
    int type=0;//0 file,1 stdout
    LogLevel::Level level=LogLevel::UNKONW;
    std::string formatter;
    std::string file;
    
    bool operator==(const LogAppenderDefine& oth)const {
        return type ==oth.type
        && level ==oth.level
        && formatter ==oth.formatter
        && file ==oth.file;
    }

};
struct LogDefine{
    std::string name;
    LogLevel::Level level=LogLevel::UNKONW;
    std::string formatter;
    std::vector<LogAppenderDefine> appenders;
    bool operator==(const LogDefine&oth) const{
        return name == oth.name
        && level == oth.level
        && formatter == oth.formatter
        && appenders == oth.appenders;
    }

    bool operator<(const LogDefine&oth) const{
        return name < oth.name;
    }
}; 

template<>
class LexicalCast<std::string,std::set<LogDefine>>{
    public:
    std::set<LogDefine> operator()(const std::string& v){
        YAML::Node node =YAML::Load(v);
        typename std::set<LogDefine> vec;
        std::stringstream ss;
        for(size_t i=0; i<node.size();++i){
            const auto& n =node[i];
            if(!n["name"].IsDefined()){
                std::cout<<"log config error: name is null,"<<n<<std::endl;
                continue;
            }
            LogDefine id;
            id.name=n["name"].as<std::string>();
            id.level= LogLevel::FromString(n["level"].IsDefined()? n["level"].as<std::string>():"");
            if(n["formatter"].IsDefined()){
                  id.formatter= n["formatter"].as<std::string>();
            }
            if(n["appenders"].IsDefined()){
                  for(size_t x=0;x<n["appenders"].size();++x){
                     auto a=n["appenders"][x];
                     if(!a["type"].IsDefined()){
                        std::cout<<"log config error: appender type is null,"<<a<<std::endl;
                        continue;
                     }
                     std::string type= a["type"].as<std::string>();
                     LogAppenderDefine lad;
                     if(type=="FileLogAppender"){
                        lad.type=1;
                        if(!a["file"].IsDefined()){
                            std::cout<<"log config error: Fileappender file is null,"<<a<<std::endl;
                            continue;
                        }
                        lad.file =a["file"].as<std::string>();
                        if(a["formatter"].IsDefined()){
                            lad.formatter=a["formatter"].as<std::string>();
                        }

                     }else if(type=="StdoutLogAppender"){
                        lad.type=2;
                        if(a["formatter"].IsDefined()){
                            lad.formatter=a["formatter"].as<std::string>();
                        }
                     }
                     else{
                        std::cout<<"log config error: appender type is invalid,"<<a<<std::endl;
                        continue;
                     }
                     id.appenders.push_back(lad);

                  }
            }
            vec.insert(id);
            
    }
            return vec;
}
};

template<>
class LexicalCast<std::set<LogDefine>,std::string>{
    public:
    std::string operator()(const std::set<LogDefine>& v){
        YAML::Node node ;
        for(auto& i:v){
            YAML::Node n;
            n["name"]=i.name;
            n["level"]=LogLevel::ToString(i.level);
            if(i.level!=LogLevel::UNKONW)
            {
                n["level"]=LogLevel::ToString(i.level);
            }
            if(!i.formatter.empty()){
                n["formatter"]=i.formatter;
            }

            for(auto&a :i.appenders){
                YAML::Node na;
                if(a.type==1){
                    na["type"]="FileLogAppender";
                    na["file"]=a.file;
                }else if(a.type==2){
                    na["type"]="StdoutLogAppender";
                }
                if(a.level!=LogLevel::UNKONW)
                {
                    na["level"]=LogLevel::ToString(a.level);
                }

                if(!a.formatter.empty()){
                    na["formatter"]=a.formatter;

                }
                n["appenders"].push_back(na);
            }
            node.push_back(n);
        }
        // for(auto& i :v){
        //     node.push_back(YAML::Load(LexicalCast<T,std::string>()(i)));
        // }
        std::stringstream ss;
        ss<<node;
        return ss.str();
    }
};

sylar::ConfigVar<std::set<LogDefine>>::ptr g_log_defines=
    sylar::Config::Lookup("logs",std::set<LogDefine>(),"logs config");

struct LogIniter{
    LogIniter(){
        g_log_defines->addListener([](const std::set<LogDefine>& old_value, const std::set<LogDefine>& new_value){
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT())<<"no_logger_conf_changed";
            for(auto& i :new_value){
            auto it=old_value.find(i);
            sylar::Logger::ptr logger;
            if(it==old_value.end()){
                //新增日志
                logger = SYLAR_LOG_NAME(i.name); 
            }else {
            if(!(i==*it)){
                //修改日志
                    logger = SYLAR_LOG_NAME(i.name);
            }
            else {
                        continue;
             }
            }
            logger ->setLevel(i.level);
            if(!i.formatter.empty())
            {
                logger->setFormatter(i.formatter);
            }
            logger->clearAppenders();
            for(auto& a : i.appenders){
                sylar::LogAppender::ptr ap;
                if(a.type==1){
                    ap.reset(new FileLogAppender(a.file));
                }else 
                if(a.type==2){
                    ap.reset(new StdoutLogAppender);
                }
                ap->setLevel(a.level);
                if(!a.formatter.empty()){
                    LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                    if(!fmt->isError()){
                        ap->setFormatter(fmt);
                    }
                    else{
                        std::cout<<"log.name="<<i.name<<"appender type="<<a.type<<"formatter"<<a.formatter<<"is invalid"<< std::endl;
                    }
                }
                logger->addAppender(ap);        
                }
            }
            for(auto& i:old_value){
                auto it =new_value.find(i);
                if(it ==new_value.end())
                {
                    //删除logger
                    auto logger = SYLAR_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::Level)100);
                    logger->clearAppenders();
                }

            }

        });
        
        }
        
        //删除日志
};    
static LogIniter __log_init;
void LoggerManager::init(){
}
}




