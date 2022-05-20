#pragma once

#include <memory>
#include <list>
#include "system.hpp"
#include "log_level.h"
#include "log_appender.h"
#include "mutex.h"

__OBELISK__

class Logger : public std::enable_shared_from_this<Logger>{
public:
    typedef std::shared_ptr<Logger> ptr;

    Logger(const char * name = "root") : m_name(name) {}

    void log(LogLevel::Level level, LogEvent::ptr event);
	void debug(LogEvent::ptr event);
	void info(LogEvent::ptr event);
	void warn(LogEvent::ptr event);
	void error(LogEvent::ptr event);
	void fatal(LogEvent::ptr event);

    void addAppender(LogAppender::ptr appender);
    void delAppender(LogAppender::ptr appender);

    LogLevel::Level getLevel() const { return m_level; }
	void setLevel(LogLevel::Level level) { m_level = level; }
	const char * getPattern() const { return m_pattern; }
	void setPattern(const char * p) { m_pattern = p; }
	const char * getName() const { return m_name; }
	void setName(const char * name) { m_name = name; }
    
	void clear();
private:
    const char * m_name = nullptr;
    const char * m_pattern = "%d [%p] %t %f:%l %m %n";
    LogLevel::Level m_level = LogLevel::DEBUG;
    std::list<LogAppender::ptr> m_appenders;
	Mutex m_mutex;		// 控制Appender的修改 
};
__END__