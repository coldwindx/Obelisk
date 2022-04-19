#pragma once

#include "utils.h"

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