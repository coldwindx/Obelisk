#pragma once

#include <stdint.h>
#include <memory>
#include <functional>
#include <map>
#include "system.h"
#include "convert.h"
#include "log/log.h"
#include "thread/mutex.h"


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
    
    T getValue() const { return m_val; }
    void setValue(const T & val);
    virtual std::string valueType() const override { return typeid(T).name(); }
    virtual std::string toString() override;
    virtual bool fromString(const std::string & str) override;

    uint64_t addListener(listener func){
        static uint64_t s_func_id = 0;
        WriteLock lock(m_mutex);
        m_listeners[++s_func_id] = func;
        return s_func_id;
    }

    void delListener(uint64_t key){
        WriteLock lock(m_mutex);
        m_listeners.erase(key);
    }

    listener getListener(uint64_t key){
        ReadLock lock(m_mutex);
        auto it = m_listeners.find(key);
        if(it == m_listeners.end())
            return nullptr;
        return it->second;
    }

    void clearListener(){
        WriteLock lock(m_mutex);
        m_listeners.clear();
    }
private:
    T m_val;
    std::map<uint64_t, listener> m_listeners;
    RWMutex m_mutex;            // 读写互斥
};

template<typename T, typename From, typename To>
void ConfigVar<T, From, To>::setValue(const T & v){
        {
            ReadLock lock(m_mutex);
            if (v == m_val) return;
            for (auto & i : m_listeners)
                i.second(m_val, v);		// 触发监听
        }
        WriteLock lock(m_mutex);
        m_val = v;
}

template<typename T, typename From, typename To>
std::string ConfigVar<T, From, To>::toString() {
	try {
        // T可能是非基本类型，所以加锁
        ReadLock lock(m_mutex);
		return To()(m_val);
	}
	catch (std::exception& e) {
		LOG_ERROR(LOG_SYSTEM()) << "ConfigVar::toString() exception " << e.what() << " convert: " << typeid(m_val).name() << " to string";
	}
	return "";
}

template<typename T, typename From, typename To>
bool ConfigVar<T, From, To>::fromString(const std::string & val) {
	try {
		T v = From()(val);
        setValue(v);
		return true;
	}
	catch (std::exception & e) {
		LOG_ERROR(LOG_SYSTEM()) << "ConfigVar::toString() exception " << e.what() << " convert: string to" << typeid(m_val).name();
	}
	return false;
}
__END__