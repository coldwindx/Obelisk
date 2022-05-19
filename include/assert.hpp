#pragma once

#include <assert.h>
#include <execinfo.h>
#include <vector>
#include "log.h"

#define OBELISK_ASSERT(x) \
    if(!(x)){   \
        LOG_ERROR(LOG_SYSTEM()) << "ASSERTION: " #x \
            << "\nbacktrace:\n" << obelisk::BacktraceToString(100, 2, "    ");  \
        assert(x);  \
    }

#define OBELISK_ASSERT2(x, w)   \
    if(!(x)){   \
        LOG_ERROR(LOG_SYSTEM()) << "ASSERTION: " #x \
            << "\n" << w << "\nbacktrace:\n" \
            << obelisk::BacktraceToString(100, 2, "    ");  \
        assert(x);  \
    }

__OBELISK__

/* 获取当前调用栈 */
static void Backtrace(std::vector<std::string>& bt, int size, int skip){
    void** array = (void**)malloc(size * sizeof(void*));
	size_t s = backtrace(array, size);

	char** strings = backtrace_symbols(array, s);
    if(strings == NULL){
        LOG_ERROR(LOG_SYSTEM()) << "backtrace_symbols error!";
        return;
    }

    for(size_t i = skip; i < s; ++i){
        bt.push_back(strings[i]);
    }
    free(strings);
    free(array);
}

static std::string BacktraceToString(int size, int skip = 1, const std::string& prefix = ""){
    std::vector<std::string> bt;
    Backtrace(bt, size, skip);
    std::stringstream ss;
    for(size_t i = 0, n = bt.size(); i < n; ++i){
        ss << prefix << bt[i] << std::endl;
    }
    return ss.str();
}

__END__