#pragma once

#include <memory>
#include <iostream>
#include <fstream>
#include "../system.h"
#include "log_level.h"
#include "log_event.h"
#include "log_formatter.h"

__SUN__
/* 日志输出 */
class LogAppender{
public:
    typedef std::shared_ptr<LogAppender> ptr;

    virtual ~LogAppender() {}

    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level level) { m_level = level; }
    LogFormatter::ptr getFormatter() const { return m_formatter; }
    void setFormatter(LogFormatter::ptr formatter) { m_formatter = formatter; }

    virtual void log(LogEvent::ptr event) = 0;
protected:
    LogLevel::Level m_level = LogLevel::DEBUG;          // 日志等级
    LogFormatter::ptr m_formatter = nullptr;
};

/* 输出到标准输出流 */
class StdoutLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<StdoutLogAppender> ptr;

    virtual void log(LogEvent::ptr event) override{
        if(event->getLevel() < m_level)
            return;
        std::cout << m_formatter->format(event);
    }
};

/* 输出到日志文件 */
class FileLogAppender : public LogAppender{
public:
    typedef std::shared_ptr<FileLogAppender> ptr;

    FileLogAppender(const std::string & filename) : m_filename(filename){}
    
    virtual void log(LogEvent::ptr event) override{
        if(event->getLevel() < m_level)
            return;
        if (event->getLevel() < this->m_level) return;
		if (m_filestream) m_filestream.close();
		m_filestream.open(m_filename, std::ios::app);
		m_filestream << m_formatter->format(event) << std::endl;
    }
private:
    std::string m_filename;
    std::ofstream m_filestream;
};
__END__