#pragma once

#include <string>
#include "../system.h"

__SUN__
// 日志级别枚举类
class LogLevel{
public:
	enum Level { UNKNOW = 0, DEBUG = 1, INFO = 2, WARN = 3, ERROR = 4, FATAL = 5 };

	static const char* toString(LogLevel::Level level) {
		switch (level) {
			case DEBUG: return "debug";
			case INFO: return "info";
			case WARN: return "warn";
			case ERROR: return "error";
			case FATAL: return "fatal";
			default: return "unknow";
		}		
	}

	static LogLevel::Level fromString(const std::string & name) {
		if (name == "debug") return DEBUG;
		if (name == "info") return INFO;
		if (name == "warn") return WARN;
		if (name == "error") return ERROR;
		if (name == "fatal") return FATAL;
		return UNKNOW;
	}
};

__END__