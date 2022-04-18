#pragma once

#include <list>
#include <map>
#include <yaml-cpp/yaml.h>
#include "../system.h"
#include "config_var.h"

__OBELISK__

class Config {
public:
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    static void loadFromYaml(const YAML::Node & node);

    template<typename T>
    static typename ConfigVar<T>::ptr lookup(const std::string & name, const T & value, const std::string& description = "");
protected:
    static void transform(const std::string & prefix
        , const YAML::Node & node
        , std::list<std::pair<std::string, YAML::Node>>& output);

private:
    static std::map<std::string, ConfigVarBase::ptr>& configVarMap();
};

template<typename T>
typename ConfigVar<T>::ptr Config::lookup(const std::string & name, const T & value, const std::string & description){
    auto it = configVarMap().find(name);
    if(configVarMap().end() != it){
        auto p = std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
        LOG_INFO(LOG_SYSTEM()) << "Find config value name= " << name;
        if(p) return p;
		LOG_ERROR(LOG_SYSTEM()) << "Lookup name= " << name
			<< " exists but type not " << typeid(T).name()
			<< ",real type is " << it->second->valueType()
			<< " " << it->second->toString();
        return nullptr;
    }
	if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._0123456789") != std::string::npos) 
		throw std::invalid_argument("Lookup name invaild: " + name);
	typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, value, description));
	configVarMap()[name] = v;
    return v;
}
__END__