#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <iomanip>
#include "log.h"
#include "env.h"

__OBELISK__

static Logger::ptr g_logger = LOG_SYSTEM();

bool Env::init(int argc, char** argv){
    char link[1024] = {0};
    char path[1024] = {0};
    sprintf(link, "/proc/%d/exe", getpid());
    readlink(link, path, sizeof(path));
    m_exe = path;

    auto pos = m_exe.find_last_of("/");
    m_cwd = m_exe.substr(0, pos).append("/");


    m_program = argv[0];
    const char* now_key = nullptr;
    for(int i = 1; i < argc; ++i){
        if('-' == argv[i][0]){
            if(1 < strlen(argv[i])){
                if(now_key){
                    add(now_key, "");
                }
                now_key = argv[i] + 1;
                continue;
            }
            LOG_ERROR(g_logger) << "invalid arg idx=" << i << " val=" << argv[i];
            return false;
        }
        if(now_key){
            add(now_key, argv[i]);
            now_key = nullptr;
            continue;
        }
        LOG_ERROR(g_logger) << "invalid arg idx=" << i << " val=" << argv[i];
        return false;
    }
    if(now_key){
        add(now_key, "");
    }
    return true;
}
void Env::add(const std::string& key, const std::string& value){
    WriteLock lock(m_mutex);
    m_args[key] = value;
}
bool Env::has(const std::string& key){
    ReadLock lock(m_mutex);
    auto it = m_args.find(key);
    return it != m_args.end();
}
std::string Env::get(const std::string& key, const std::string& default_value){
    ReadLock lock(m_mutex);
    auto it = m_args.find(key);
    return it == m_args.end() ? default_value : it->second;
}
void Env::remove(const std::string& key){
    WriteLock lock(m_mutex);
    m_args.erase(key);
}
void Env::addHelp(const std::string& key, const std::string& desc){
    WriteLock lock(m_mutex);
    for(auto it = m_helps.begin(); it != m_helps.end(); ){
        if(it->first == key)
            it = m_helps.erase(it);
        else
            ++it;
    }
    m_helps.push_back(std::make_pair(key, desc));
}
void Env::removeHelp(const std::string& key){
    WriteLock lock(m_mutex);
    for(auto it = m_helps.begin(); it != m_helps.end(); ){
        if(it->first == key)
            it = m_helps.erase(it);
        else
            ++it;
    }
}
void Env::printHelp(){
    ReadLock lock(m_mutex);
    std::cout << "Usage: " << m_program << " [option]" << std::endl;
    for(auto & i : m_helps){
        std::cout << std::setw(5) << "-" << i.first << " : " << i.second << std::endl;
    }
}

std::string Env::getAbsolutePath(const std::string& path) const{
    if(path.empty())
        return "/";
    if('/' == path[0])
        return path;
    return m_cwd + path;
}

std::string Env::getConfigPath(){
    return getAbsolutePath(get("c", "conf"));
}

bool Env::setEnv(const std::string& key, const std::string& val){
    return !setenv(key.c_str(), val.c_str(), 1);
}
std::string Env::getEnv(const std::string& key, const std::string& default_value){
    const char* v = getenv(key.c_str());
    if(v == nullptr)
        return default_value;
    return v;
}
__END__