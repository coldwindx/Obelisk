#pragma once

#include <map>
#include "system.hpp"
#include "logger.h"
#include "mutex.h"

__OBELISK__
class LoggerManager {
public:
    typedef std::shared_ptr<LoggerManager> ptr;

    void set(const std::string& name, Logger::ptr logger){
        SpinLock lock();
        
        m_loggers[name] = logger;
    }
    Logger::ptr get(const std::string& name = "system") {
        SpinLock lock();

        auto it = m_loggers.find(name);
        if(m_loggers.end() != it)
            return it->second;
        Logger::ptr logger(new Logger(name.c_str()));
        return m_loggers[name] = logger;
    }
    void del(const std::string & name){
        SpinLock lock();

        m_loggers.erase(name);
    }
    static LoggerManager::ptr instance(){
        static LoggerManager::ptr p(new LoggerManager());
        return p;
    }
private:
    std::map<std::string, Logger::ptr> m_loggers;
    //Mutex m_mutex;

    LoggerManager(){
        Logger::ptr logger(new Logger());
        LogAppender::ptr appender(new FileLogAppender("system.log"));
        LogFormatter::ptr formatter(new LogFormatter(logger->getPattern()));
        
        appender->setFormatter(formatter);
        logger->addAppender(appender);
        m_loggers["system"] = logger;
    }
};
__END__
