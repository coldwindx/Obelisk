#pragma once

#include <stdio.h>  
#include <stdarg.h> 
#include "../system.h"
#include "log_event.h"
#include "logger.h"

__OBELISK__

class LogWriter{
public:
    LogWriter(Logger::ptr logger, LogEvent::ptr event) : m_logger(logger), m_event(event) {}
	~LogWriter() {
		m_logger->log(m_event->getLevel(), m_event);
	}

	std::stringstream& stream() { return m_event->stream(); }
	std::stringstream& format(const char * fmt, ...) {
		va_list al;
		va_start(al, fmt);
		char buf[1024];
		int len = vsnprintf(buf, 1024, fmt, al);
		if (len != -1) m_event->stream() << std::string(buf, len);
		va_end(al);
	}
private:
    Logger::ptr m_logger;
    LogEvent::ptr m_event;
};

__END__