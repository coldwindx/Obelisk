#pragma once

#include <map>
#include <functional>
#include "../system.h"
#include "format.h"

__SUN__
/* 格式转换管理器 */
class FormatManager{
public:
    typedef std::shared_ptr<FormatManager> ptr;
    typedef std::function<Format::ptr(const std::string&)> build;
    // C++11后由编译器保证其线程安全
    static FormatManager::ptr instance(){
        static FormatManager::ptr p(new FormatManager());
        return p;
    }

    void set(const std::string & name, build func){
        m_fmtBuildMap[name] = func;
    }

    Format::ptr get(const std::string & name, const std::string & fmt = ""){
        return m_fmtBuildMap[name](fmt);
    }

    bool has(const std::string & name) const{
        return 0 < m_fmtBuildMap.count(name);
    }
private:
    std::map<std::string, build> m_fmtBuildMap;

    FormatManager(){
        m_fmtBuildMap["m"] = [](const std::string & fmt) { return Format::ptr(new MessageFormat(fmt)); };
		m_fmtBuildMap["p"] = [](const std::string & fmt) { return Format::ptr(new LevelFormat(fmt)); };
		m_fmtBuildMap["r"] = [](const std::string & fmt) { return Format::ptr(new ElapseFormat(fmt)); };
		m_fmtBuildMap["c"] = [](const std::string & fmt) { return Format::ptr(new LoggerNameFormat(fmt)); };
		m_fmtBuildMap["t"] = [](const std::string & fmt) { return Format::ptr(new ThreadIdFormat(fmt)); };
		m_fmtBuildMap["n"] = [](const std::string & fmt) { return Format::ptr(new EnterFormat(fmt)); };
		m_fmtBuildMap["d"] = [](const std::string & fmt) { return Format::ptr(new DateTimeFormat(fmt)); };
		m_fmtBuildMap["f"] = [](const std::string & fmt) { return Format::ptr(new FilenameFormat(fmt)); };
		m_fmtBuildMap["l"] = [](const std::string & fmt) { return Format::ptr(new LineFormat(fmt)); };
		m_fmtBuildMap["T"] = [](const std::string & fmt) { return Format::ptr(new TabFormat(fmt)); };
		m_fmtBuildMap["F"] = [](const std::string & fmt) { return Format::ptr(new FiberIdFormat(fmt)); };
    }
};

__END__
