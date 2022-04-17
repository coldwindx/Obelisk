#include "logger.h"

__OBELISK__

void Logger::log(LogLevel::Level level, LogEvent::ptr event){
    event->setLevel(level);
    event->setLoggerName(m_name);
    if(level < this->m_level)
        return;
    for(auto ap : m_appenders)
        ap->log(event);
}
void Logger::debug(LogEvent::ptr event){
    log(LogLevel::DEBUG, event);
}
void Logger::info(LogEvent::ptr event){
    log(LogLevel::INFO, event);
}
void Logger::warn(LogEvent::ptr event){
    log(LogLevel::WARN, event);
}
void Logger::error(LogEvent::ptr event){
    log(LogLevel::ERROR, event);
}
void Logger::fatal(LogEvent::ptr event){
    log(LogLevel::FATAL, event);
}

void Logger::addAppender(LogAppender::ptr appender){
    m_appenders.push_back(appender);
}
void Logger::delAppender(LogAppender::ptr appender){
    for(auto it = m_appenders.begin(); it != m_appenders.end(); ++it)
        if(*it == appender){
            m_appenders.erase(it);
            break;
        }
}

void Logger::clear(){
    m_name = nullptr;
	m_level = LogLevel::DEBUG;
	m_pattern = "%d [%p] %t %f:%l %m %n";
	m_appenders.clear();
}
__END__