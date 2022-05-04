#include "system.hpp"
#include "config.h"
#include "log.h"

__OBELISK__

void Config::loadFromYaml(const YAML::Node & root){
    std::list<std::pair<std::string, YAML::Node> > nodes;
    transform("", root, nodes);
    for(auto & i : nodes){
        std::string key = i.first;
        if(key.empty()) continue;

        ReadLock lock(GetMutex());
        auto it = configVarMap().find(key);
        if(configVarMap().end() == it)
            continue;
        if(i.second.IsScalar()){
            it->second->fromString(i.second.Scalar());
        }else{
            std::stringstream ss;
            ss << i.second;
            it->second->fromString(ss.str());
        }
    }
}

void Config::visit(std::function<void(ConfigVarBase::ptr)> callback){
    ReadLock lock(GetMutex());
    for(auto it = configVarMap().begin();
                it != configVarMap().end(); ++it)
        callback(it->second);
}

void Config::transform(const std::string & prefix, const YAML::Node & node
        , std::list<std::pair<std::string, YAML::Node>>& output){
	if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ._0123456789") != std::string::npos) {
		LOG_ERROR(LOG_SYSTEM()) << "Config invaild name: " << prefix << " : " << node;
		throw std::invalid_argument(prefix);
	}
	output.push_back(std::make_pair(prefix, node));
	if (node.IsMap()) {
		for (auto it = node.begin(); it != node.end(); ++it) {
			transform(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
		}
	}
}

std::map<std::string, ConfigVarBase::ptr>& Config::configVarMap(){
	static ConfigVarMap _configVarMap;
	return _configVarMap;
}

RWMutex& Config::GetMutex(){
    static RWMutex s_mutex;
    return s_mutex;
}

__END__