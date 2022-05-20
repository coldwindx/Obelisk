#pragma once

#include "thread.h"
#include "coroutine.h"

#define LOG_LEVEL(logger, level) \
	if(logger->getLevel() <= level) \
        obelisk::LogWriter(logger, obelisk::LogEvent::ptr(new obelisk::LogEvent(__FILE__, __LINE__, level, 0, obelisk::thread_id(), obelisk::Thread::GetName(), obelisk::Coroutine::GetCoroutineId(), time(0)))).stream()
#define LOG_DEBUG(logger) LOG_LEVEL(logger, obelisk::LogLevel::DEBUG)
#define LOG_INFO(logger) LOG_LEVEL(logger, obelisk::LogLevel::INFO)
#define LOG_WARN(logger) LOG_LEVEL(logger, obelisk::LogLevel::WARN)
#define LOG_ERROR(logger) LOG_LEVEL(logger, obelisk::LogLevel::ERROR)
#define LOG_FATAL(logger) LOG_LEVEL(logger, obelisk::LogLevel::FATAL)

#define LOG_FMT_LEVEL(logger, level, fmt, ...) \
	if(logger->getLevel() <= level) \
		obelisk::LogWriter(logger, obelisk::LogEvent::ptr(new obelisk::LogEvent(__FILE__, __LINE__, level, 0, obelisk::thread_id(), obelisk::Thread::GetName(), obelisk::Coroutine::GetCoroutineId(), time(0)))).format(fmt, __VA_ARGS__)
#define LOG_FMT_DEBUG(logger, fmt, ...) LOG_FMT_LEVEL(logger, obelisk::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define LOG_FMT_INFO(logger, fmt, ...) LOG_FMT_LEVEL(logger, obelisk::LogLevel::INFO, fmt, __VA_ARGS__)
#define LOG_FMT_WARN(logger, fmt, ...) LOG_FMT_LEVEL(logger, obelisk::LogLevel::WARN, fmt, __VA_ARGS__)
#define LOG_FMT_ERROR(logger, fmt, ...) LOG_FMT_LEVEL(logger, obelisk::LogLevel::ERROR, fmt, __VA_ARGS__)
#define LOG_FMT_FATAL(logger, fmt, ...) LOG_FMT_LEVEL(logger, obelisk::LogLevel::FATAL, fmt, __VA_ARGS__)

#define LOG_SYSTEM() obelisk::LoggerManager::instance()->get()
#define LOG_NAME(name) obelisk::LoggerManager::instance()->get(name)

