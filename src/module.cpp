#include "log.h"
#include "config.h"
#include "env.h"
#include "utils.h"
#include "library.h"
#include "module.h"

__OBELISK__

static Logger::ptr g_logger = LOG_SYSTEM();

static ConfigVar<std::string>::ptr g_module_path 
        = Config::Lookup("module.path", std::string("module"), "module path");

Module::Module(const std::string& name, const std::string& version, const std::string& filename)
            : m_name(name), m_version(version), m_filename(filename) {
    m_id = name + "/" + version;
}

bool Module::onLoad(){
    return true;
}
bool Module::onUnload(){
    return true;
}
bool Module::onConnect(Stream::ptr stream){
    return true;
}
bool Module::onDisconnect(Stream::ptr stream){
    return true;
}
bool Module::onServerReady(){
    return true;
}
bool Module::onServerUp(){
    return true;
}
bool Module::beforeArgsParse(int argc, char** argv){
    return true;
}
bool Module::afterArgsParse(int argc, char** argv){
    return true;
}
ModuleManager* ModuleManager::instance(){
    static ModuleManager *manager = new ModuleManager();
    return manager;
}

void ModuleManager::init(){
    auto path = Env::instance()->getAbsolutePath(g_module_path->getValue());

    std::vector<std::string> files;
    FileUtils::ListAllFile(files, path, ".so");
    std::sort(files.begin(), files.end());
    for(auto & file : files){
        Module::ptr m = Library::Load(file);
        if(m) add(m);
    }
}
void ModuleManager::add(Module::ptr module){
    del(module->getId());
    WriteLock lock(m_mutex);
    m_modules[module->getId()] = module;
}
void ModuleManager::del(const std::string& name){
    Module::ptr module;
    WriteLock lock(m_mutex);
    auto it = m_modules.find(name);
    if(it == m_modules.end())
        return;
    module = it->second;
    m_modules.erase(it);
    lock.unlock();
    module->onUnload();
}
void ModuleManager::clear(){
    ReadLock lock(m_mutex);
    auto tmp = m_modules;
    lock.unlock();
    for(auto & i : tmp)
        del(i.first);
}
Module::ptr ModuleManager::get(const std::string& name){
    ReadLock lock(m_mutex);
    auto it =   m_modules.find(name);
    return it == m_modules.end() ? nullptr : it->second;
}
void ModuleManager::list(std::vector<Module::ptr>& modules){
    ReadLock lock(m_mutex);
    for(auto & m : m_modules)
        modules.push_back(m.second);
}


__END__