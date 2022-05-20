#pragma once

#include <memory>
#include <sstream>
#include "system.hpp"
#include "log_level.h"

__OBELISK__

/* 日志事件类 */
class LogEvent{
public:
    typedef std::shared_ptr<LogEvent> ptr;

	LogEvent(const char * filename
		, int32_t line
		, LogLevel::Level level
		, uint32_t elapse
		, uint32_t threadId
        , const std::string & threadName
		, uint32_t coroutineId
		, uint64_t time) {
		this->m_filename = filename;
		this->m_line = line;
		this->m_level = level;
		this->m_elapse = elapse;
		this->m_threadId = threadId;
        this->m_threadName = threadName;
		this->m_coroutineId = coroutineId;
		this->m_time = time;
	}

    const char * getFilename() const { return m_filename; }
    uint32_t getLine() const { return m_line; }
    uint32_t getThreadId() const { return m_threadId; }
    std::string& getThreadName() { return m_threadName; }
    uint32_t getCoroutineId() const { return m_coroutineId; }
    uint32_t getTime() const { return m_time; }
    uint32_t getElapse() const { return m_elapse; }
    LogLevel::Level getLevel() const { return m_level; }
    void setLevel(LogLevel::Level level) { m_level = level; }
    std::stringstream& stream() { return m_stream; }
    std::string getLoggerName() const { return m_loggerName; }
    void setLoggerName(const std::string & name) { m_loggerName = name; }
private:
    const char * m_filename = nullptr;  // 文件名
    uint32_t m_line = 0;                // 行号
    uint32_t m_threadId = 0;            // 线程ID
    std::string m_threadName = "";      
    uint32_t m_coroutineId = 0;         // 协程ID
    uint32_t m_time = 0;                // 时间戳
    uint32_t m_elapse = 0;              // 运行时间
    LogLevel::Level m_level;            // 日志等级
    std::stringstream m_stream;         // 日志流
    std::string m_loggerName;           // 日志输出器
};

__END__