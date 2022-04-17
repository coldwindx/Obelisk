#pragma once

#include <stdint.h>
#include <thread>
#include <sstream>
#include <time.h>

#define __OBELISK__ namespace obelisk {
#define __END__ }

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