#pragma once

#include "system.h"
#include "log/log_writer.h"
#include "log/logger_manager.h"

__SUN__

#define LOG_LEVEL(logger, level) \
	if(logger->getLevel() <= level) \
        LogWriter(logger, LogEvent::ptr(new LogEvent(__FILE__, __LINE__, level, 0, thread_id(), fiber_id(), time(0)))).stream()
#define LOG_DEBUG(logger) LOG_LEVEL(logger, LogLevel::DEBUG)
#define LOG_INFO(logger) LOG_LEVEL(logger, LogLevel::INFO)
#define LOG_WARN(logger) LOG_LEVEL(logger, LogLevel::WARN)
#define LOG_ERROR(logger) LOG_LEVEL(logger, LogLevel::ERROR)
#define LOG_FATAL(logger) LOG_LEVEL(logger, LogLevel::FATAL)

#define LOG_FMT_LEVEL(logger, level, fmt, ...) \
	if(logger->getLevel() <= level) \
		LogWriter(logger, LogEvent::ptr(new LogEvent(__FILE__, __LINE__, level, 0, :thread_id(), fiber_id(), time(0)))).format(fmt, __VA_ARGS__)
#define LOG_FMT_DEBUG(logger, fmt, ...) LOG_FMT_LEVEL(logger, LogLevel::DEBUG, fmt, __VA_ARGS__)
#define LOG_FMT_INFO(logger, fmt, ...) LOG_FMT_LEVEL(logger, LogLevel::INFO, fmt, __VA_ARGS__)
#define LOG_FMT_WARN(logger, fmt, ...) LOG_FMT_LEVEL(logger, LogLevel::WARN, fmt, __VA_ARGS__)
#define LOG_FMT_ERROR(logger, fmt, ...) LOG_FMT_LEVEL(logger, LogLevel::ERROR, fmt, __VA_ARGS__)
#define LOG_FMT_FATAL(logger, fmt, ...) LOG_FMT_LEVEL(logger, LogLevel::FATAL, fmt, __VA_ARGS__)

#define LOG_SYSTEM() LoggerManager::instance()->get()
#define LOG_NAME(name) LoggerManager::instance()->get(name)

__END__