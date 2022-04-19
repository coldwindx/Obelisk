#include "../system.h"
#include "../config/config.h"
#include "log_event.h"
#include "log_writer.h"
#include "logger_manager.h"
/* 日志系统对于配置系统的兼容 */
__OBELISK__

struct LogAppenderConfig {
	std::string type;
	LogLevel::Level level = LogLevel::UNKNOW;
	std::string format;
	std::string file;

	bool operator==(const LogAppenderConfig & oth) const {
		return type == oth.type && level == oth.level
			&& format == oth.format && file == oth.file;
	}
};

struct LogConfig {
	std::string name;
	LogLevel::Level level = LogLevel::UNKNOW;
	std::string format;
	std::vector<LogAppenderConfig> appenders;

	bool operator==(const LogConfig & oth) const {
		return name == oth.name && level == oth.level
			&& format == oth.format && appenders == oth.appenders;
	}

	bool operator<(const LogConfig& oth) const {
		return name < oth.name;
	}
};


template<>
class Convert<std::string, LogConfig> {
public:
	LogConfig operator()(const std::string& v) {
		YAML::Node n = YAML::Load(v);
		LogConfig ld;
		if (!n["name"].IsDefined()) {
			//std::cout << "log config error: name is null, " << n
			//	<< std::endl;
			throw std::logic_error("log config name is null");
		}
		ld.name = n["name"].as<std::string>();
		ld.level = LogLevel::fromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
		if (n["format"].IsDefined()) {
			ld.format = n["format"].as<std::string>();
		}

		if (n["appenders"].IsDefined()) {
			for (size_t x = 0; x < n["appenders"].size(); ++x) {
				auto a = n["appenders"][x];
				if (!a["type"].IsDefined()) {
					throw std::logic_error("log config error: appender type is null in logger= " + ld.name);
					continue;
				}
				LogAppenderConfig lad;
				lad.type = a["type"].as<std::string>();
				if (lad.type == "FileLogAppender") {
					if (!a["file"].IsDefined()) {
						throw std::logic_error("log config error: fileappender file is null in logger=" + ld.name);
						continue;
					}
					lad.file = a["file"].as<std::string>();
					if (a["format"].IsDefined()) {
						lad.format = a["format"].as<std::string>();
					}
				}
				else if (lad.type == "StdoutLogAppender") {
					if (a["format"].IsDefined()) {
						lad.format = a["format"].as<std::string>();
					}
				}
				else {
					throw std::logic_error("log config error: appender type is invalid in logger=" + ld.name);
					continue;
				}

				if (a["level"].IsDefined())
					lad.level = LogLevel::fromString(a["level"].as<std::string>());
				else
					lad.level = ld.level;

				ld.appenders.push_back(lad);
			}
		}
		return ld;
	}
};

template<>
class Convert<LogConfig, std::string> {
public:
	std::string operator()(const LogConfig& i) {
		YAML::Node n;
		n["name"] = i.name;
		if (i.level != LogLevel::UNKNOW) {
			n["level"] = LogLevel::toString(i.level);
		}
		if (!i.format.empty()) {
			n["format"] = i.format;
		}

		for (auto& a : i.appenders) {
			YAML::Node na(YAML::NodeType::Map);
			na["type"] = "FileLogAppender";
			if(!a.file.empty()) na["file"] = a.file;
			if (a.level != LogLevel::UNKNOW) {
				na["level"] = LogLevel::toString(a.level);
			}

			if (!a.format.empty()) {
				na["format"] = a.format;
			}

			n["appenders"].push_back(na);
		}
		std::stringstream ss;
		ss << n;
		return ss.str();
	}
};

struct LogIniter{
    ConfigVar<std::set<LogConfig> >::ptr g_log_defines = Config::lookup("logs", std::set<LogConfig>(), "logs config");

    LogIniter(){
        g_log_defines->addListener([](const std::set<LogConfig>& oldValue, const std::set<LogConfig>& newValue){

            LoggerManager::ptr manager = LoggerManager::instance();

            for(auto & i : newValue){
                auto it = oldValue.find(i);
                if(oldValue.end() != it && i == *it)
                    continue;
                Logger::ptr logger = manager->get(i.name);
                logger->clear();

                logger->setName(i.name.c_str());
                logger->setLevel(i.level);
                if(!i.format.empty())
                    logger->setPattern(i.format.c_str());
                for(auto & a : i.appenders){
                    LogAppender::ptr ap;
                    if("FileLogAppender" == a.type)
                        ap.reset(new FileLogAppender(a.file));
                    if("StdoutLogAppender" == a.type)
                        ap.reset(new StdoutLogAppender());
                    ap->setLevel(LogLevel::UNKNOW == a.level ? i.level : a.level);

                    std::string fmt = a.format.empty() ? logger->getPattern() : a.format;
                    LogFormatter::ptr format(new LogFormatter(fmt));
                    if(!format->isError()){
                        ap->setFormatter(format);
                    }else{
                        LogWriter writer(manager->get("system")
                                , LogEvent::ptr(new LogEvent(__FILE__, __LINE__, LogLevel::ERROR, 0, thread_id(), fiber_id(), time(0))));
                    	writer.stream() << "logger name=" << i.name
								<< " appender type=" << a.type
								<< " format=" << a.format
								<< " is invaild" << std::endl;   
                    }
                    logger->addAppender(ap);
                }
            }

            for(auto & i : oldValue){
                auto it = newValue.find(i);
                if(it == newValue.end())
                    manager->del(i.name);
            }
        });
    }
}__log_init;

__END__
