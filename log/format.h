#pragma once

#include <memory>
#include <iosfwd>
#include <time.h>
#include "../system.h"
#include "log_level.h"
#include "log_event.h"

__OBELISK__
/* 日志信息格式器 */
class Format{
public:
    typedef std::shared_ptr<Format> ptr;

    Format(const std::string& fmt = "") {}
    virtual ~Format() {}

    virtual void format(std::ostream & os, LogEvent::ptr event) = 0;
private:
};

/*--------------------------------消息体格式化--------------------------------------*/
class MessageFormat: public Format{
public:
    MessageFormat(const std::string& fmt = "") {}
    virtual void format(std::ostream & os, LogEvent::ptr event) override{
        os << event->stream().str();
    }
};

class EnterFormat : public Format{
public:
    EnterFormat(const std::string& fmt = "") {}
    virtual void format(std::ostream & os, LogEvent::ptr event) override{
        os << '\n';
    }
};

class TabFormat : public Format{
public:
    TabFormat(const std::string& fmt = "") {}
    virtual void format(std::ostream & os, LogEvent::ptr event) override{
        os << '\t';
    }
};

class StringFormat : public Format{
public:
    StringFormat(const std::string& fmt = "") : m_str(fmt) {}
    virtual void format(std::ostream & os, LogEvent::ptr event) override{
        os << m_str;
    }
private:
    std::string m_str;
};

/*--------------------------------日志信息格式化--------------------------------------*/
class LevelFormat : public Format {
public:
	LevelFormat(const std::string & fmt = "") {}
	// 通过 Format 继承
	virtual void format(std::ostream & os, LogEvent::ptr event) override {
		os << LogLevel::toString(event->getLevel());
	}
};

class LoggerNameFormat : public Format {
public:
	LoggerNameFormat(const std::string & fmt = "") {}
	// 通过 Format 继承
	virtual void format(std::ostream & os, LogEvent::ptr event) override {
		os << event->getLoggerName();
	}
};

/*--------------------------------程序信息格式化--------------------------------------*/
class ElapseFormat : public Format {
public:
	ElapseFormat(const std::string & fmt = "") {}
	// 通过 Format 继承
	virtual void format(std::ostream & os, LogEvent::ptr event) override {
		os << event->getElapse();
	}
};

class LineFormat : public Format {
public:
	LineFormat(const std::string & fmt = "") {}
	// 通过 Format 继承
	virtual void format(std::ostream & os, LogEvent::ptr event) override {
		os << event->getLine();
	}
};


/*--------------------------------线程信息格式化--------------------------------------*/
class ThreadIdFormat : public Format {
public:
	ThreadIdFormat(const std::string & fmt = "") {}
	// 通过 Format 继承
	virtual void format(std::ostream & os, LogEvent::ptr event) override {
		os << std::hex << event->getThreadId() << std::dec;
	}
};

/*--------------------------------协程信息格式化--------------------------------------*/
class FiberIdFormat : public Format {
public:
	FiberIdFormat(const std::string & fmt = "") {}
	// 通过 Format 继承
	virtual void format(std::ostream & os, LogEvent::ptr event) override {
		os << event->getFiberId();
	}
};

/*--------------------------------时间信息格式化--------------------------------------*/
class DateTimeFormat : public Format {
public:
	DateTimeFormat(const std::string & format) {
		m_fmt = format.empty() ? "%Y-%m-%d %H:%M:%S" : format;
	}
	// 通过 Format 继承
	virtual void format(std::ostream & os, LogEvent::ptr event) override {
		time_t time = event->getTime();
		struct tm tm;
		char buf[64];
        localtime_r(&time, &tm);
		strftime(buf, sizeof(buf), m_fmt.c_str(), &tm);
		os << buf;
	}
private:
	std::string m_fmt;
};

/*--------------------------------文件信息格式化--------------------------------------*/
class FilenameFormat : public Format {
public:
	FilenameFormat(const std::string & fmt = "") {}
	// 通过 Format 继承
	virtual void format(std::ostream & os, LogEvent::ptr event) override {
		os << event->getFilename();
	}
};
__END__