#pragma once

#include <list>
#include <map>
#include <yaml-cpp/yaml.h>
#include "system.h"
#include "config_var.h"

__OBELISK__

class Config {
public:
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    static void loadFromYaml(const YAML::Node & node);
protected:
    static void transform(const std::string & prefix
        , const YAML::Node & node
        , std::list<std::pair<std::string, YAML::Node>>& output);

private:
    static std::map<std::string, ConfigVarBase::ptr>& configVarMap();
};

__END__