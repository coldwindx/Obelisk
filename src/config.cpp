#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "env.h"
#include "log.h"
#include "utils.h"
#include "config.h"

__OBELISK__

static Logger::ptr g_logger = LOG_SYSTEM();

void Config::LoadFromYaml(const YAML::Node & root){
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

static std::map<std::string, uint64_t> s_file2modifytime;
static Mutex s_mutex;

void Config::LoadFromConfDir(const std::string& path){
    std::string absoulte_path = Env::instance()->getAbsolutePath(path);
    std::vector<std::string> files;
    FileUtils::ListAllFile(files, absoulte_path, ".yaml");

    for(auto & i : files){
        struct stat st;
        lstat(i.c_str(), &st);
        {
            Lock lock(s_mutex);
            if(st.st_mtime == s_file2modifytime[i])
                continue;
            s_file2modifytime[i] = st.st_mtime;
        }
        try{
            YAML::Node root = YAML::LoadFile(i);
            LoadFromYaml(root);
            LOG_INFO(g_logger) << "LoadConfFile file=" << i << " success";
        }catch(...){
            LOG_ERROR(g_logger) << "LoadConfFile file=" << i << " failed";
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