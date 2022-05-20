#pragma once

#include <memory>
#include "module.h"

__OBELISK__

class Library{
public:
    static Module::ptr Load(const std::string& path);
};

__END__