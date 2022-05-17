#pragma once

#include <vector>
#include "system.hpp"

__OBELISK__

class FileUtils{
public:
    static void ListAllFile(std::vector<std::string>& files
                            , const std::string& path
                            , const std::string& subfix);
};

__END__