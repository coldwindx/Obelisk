#pragma once

#include <memory>
#include <vector>
#include "../system.h"
#include "log_event.h"
#include "format.h"

__OBELISK__
/* 日志格式输出 */
class LogFormatter {
public:
	typedef std::shared_ptr<LogFormatter> ptr;
	LogFormatter(const std::string & pattern);

	void init();
	std::string format(LogEvent::ptr event);
	bool isError()const { return m_error; }
private:
	bool m_error = false;
	std::string m_pattern;
	std::vector<Format::ptr> m_formats;
};
__END__