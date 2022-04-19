#pragma once

#include <stdint.h>
#include <time.h>
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

static uint32_t fiber_id() {
	return 0;
}

__END__