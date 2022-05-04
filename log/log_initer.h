#pragma once

#include "system.hpp"
#include "log_event.h"
#include "log_writer.h"
#include "logger_manager.h"
/* 日志系统对于配置系统的兼容 */
__OBELISK__

struct LogAppenderConfig {
	std::string type;
	LogLevel::Level level = LogLevel::UNKNOW;
	std::string format;
	std::string file;

	bool operator==(const LogAppenderConfig & oth) const {
		return type == oth.type && level == oth.level
			&& format == oth.format && file == oth.file;
	}
};

struct LogConfig {
	std::string name;
	LogLevel::Level level = LogLevel::UNKNOW;
	std::string format;
	std::vector<LogAppenderConfig> appenders;

	bool operator==(const LogConfig & oth) const {
		return name == oth.name && level == oth.level
			&& format == oth.format && appenders == oth.appenders;
	}

	bool operator<(const LogConfig& oth) const {
		return name < oth.name;
	}
};

struct LogIniter{
	LogIniter();
};

__END__
