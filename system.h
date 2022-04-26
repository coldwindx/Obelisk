#pragma once

#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <sstream>
#include <thread>

#define __OBELISK__ namespace obelisk {
#define __END__ }

#define _USE_THREAD_ 1		// 启用线程安全

__OBELISK__

static uint32_t thread_id(){
    std::stringstream ss;
	ss << std::this_thread::get_id();
	return std::stoull(ss.str());
}

static uint64_t GetCurrentMS(){
	static timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}

static uint64_t GetCurrentUS(){
	static timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
}
__END__