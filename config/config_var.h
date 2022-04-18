#pragma once

#include <memory>
#include <functional>
#include <map>
#include "../system.h"
#include "../log/log.h"
#include "convert.h"


__OBELISK__

class ConfigVarBase{
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;

    ConfigVarBase(const std::string & name, const std::string & description = "")
        : m_name(name), m_description(description) {}
    virtual ~ConfigVarBase() {}

    std::string name() const { return m_name; }
    std::string description() const { return m_description; }

    virtual std::string valueType() const = 0;
    virtual std::string toString() = 0;
    virtual bool fromString(const std::string & val) = 0;
private:
    std::string m_name;
    std::string m_description;
};

template<typename T, 
    typename From = Convert<std::string, T>, 
    typename To = Convert<T, std::string> >
class ConfigVar : public ConfigVarBase{
public:
    typedef std::shared_ptr<ConfigVar> ptr;
    typedef std::function<void(const T & oldV, const T & newV)> listener;

    ConfigVar(const std::string & name, const T & value, const std::string& description = "")
        : ConfigVarBase(name, description), m_val(value) {}
    
    T value() const { return m_val; }
    virtual std::string valueType() const override { return typeid(T).name(); }
    virtual std::string toString() override;
    virtual bool fromString(const std::string & str) override;

    void addListener(uint64_t key, listener func){
        m_listeners[key] = func;
    }

    void delListener(uint64_t key){
        m_listeners.erase(key);
    }

    listener getListener(uint64_t key){
        auto it = m_listeners.find(key);
        if(it == m_listeners.end())
            return nullptr;
        return it->second;
    }

    void clearListener(){
        m_listeners.clear();
    }
private:
    T m_val;
    std::map<uint64_t, listener> m_listeners;
};


template<typename T, typename From, typename To>
std::string ConfigVar<T, From, To>::toString() {
	try {
		return To()(m_val);
	}
	catch (std::exception& e) {
		LOG_ERROR(LOG_SYSTEM()) << "ConfigVar::toString() exception" << e.what() << " convert: " << typeid(m_val).name() << " to string";
	}
	return "";
}

template<typename T, typename From, typename To>
bool ConfigVar<T, From, To>::fromString(const std::string & val) {
	try {
		T v = From()(val);
		if (v == m_val) return true;
		for (auto & i : m_listeners)
			i.second(m_val, v);		// 触发监听
		m_val = v;
		return true;
	}
	catch (std::exception & e) {
		LOG_ERROR(LOG_SYSTEM()) << "ConfigVar::toString() exception" << e.what() << " convert: string to" << typeid(m_val).name();
	}
	return false;
}
__END__