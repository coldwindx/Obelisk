#pragma once

#include <list>
#include <map>
#include <functional>
#include <yaml-cpp/yaml.h>
#include "system.hpp"
#include "config_var.h"
#include "log.h"
#include "mutex.h"

__OBELISK__

class Config {
public:
    typedef std::map<std::string, ConfigVarBase::ptr> ConfigVarMap;
    /**
     * @brief 由yaml配置文件获取配置信息
     */
    static void LoadFromYaml(const YAML::Node & node);
    /**
     * @brief 由文件夹批量获取配置信息
     */
    static void LoadFromConfDir(const std::string& path);

    template<typename T>
    static typename ConfigVar<T>::ptr Lookup(const std::string & name);
    template<typename T>
    static typename ConfigVar<T>::ptr Lookup(const std::string & name, const T & value, const std::string& description = "");

    static void visit(std::function<void(ConfigVarBase::ptr)> callback);
protected:
    static void transform(const std::string & prefix
        , const YAML::Node & node
        , std::list<std::pair<std::string, YAML::Node>>& output);

private:
    static std::map<std::string, ConfigVarBase::ptr>& configVarMap();
    static RWMutex& GetMutex();
};


template<typename T>
typename ConfigVar<T>::ptr Config::Lookup(const std::string & name){
    ReadLock lock(GetMutex());
    auto it = configVarMap().find(name);
    if(configVarMap().end() == it){
        return nullptr;
    }
    return std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
}

template<typename T>
typename ConfigVar<T>::ptr Config::Lookup(const std::string & name, const T & value, const std::string & description){
    WriteLock lock(GetMutex());
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