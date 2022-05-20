#include <dlfcn.h>
#include "log.h"
#include "config.h"
#include "env.h"
#include "library.h"

__OBELISK__

static Logger::ptr g_logger = LOG_SYSTEM();

typedef Module* (*create_module)();
typedef void (*destroy_module)(Module*);
/**
 * @brief Module智能指针释放伪函数
 */
class ModuleCloser{
public:
    ModuleCloser(void *handle, destroy_module destroy)
        : m_handle(handle), m_destroy(destroy) {}
    
    void operator()(Module * module){
        std::string name = module->getName();
        std::string version = module->getVersion();
        std::string path = module->getFilename();

        m_destroy(module);
        int rt = dlclose(m_handle);

        if(rt){
            LOG_ERROR(g_logger) << "dlclose handle fail handle=" << m_handle
                << " name=" << name << " version=" << version << " path=" << path
                << " error=" << dlerror();
            return; 
        }
        LOG_INFO(g_logger) << "destroy module name=" << name 
                << " version=" << version << " path=" << path << "success";
    }
private:
    void *m_handle;
    destroy_module m_destroy;
};

Module::ptr Library::Load(const std::string& path){
    void *handle = dlopen(path.c_str(), RTLD_NOW);
    if(!handle){
        LOG_ERROR(g_logger) << "cannot load library path=" << path
            << " error=" << dlerror();
        return nullptr;
    }

    create_module create = (create_module)dlsym(handle, "CreateModule");
    if(!create){
        LOG_ERROR(g_logger) << "cannot load symbol CreateModule in" << path
            << " error=" << dlerror();
        return nullptr;
    }

    destroy_module destroy = (destroy_module)dlsym(handle, "DestoryModule");
    if(!destroy){
        LOG_ERROR(g_logger) << "cannot load symbol DestoryModule in" << path
            << " error=" << dlerror();
        dlclose(handle);
        return nullptr;
    }

    Module::ptr module(create(), ModuleCloser(handle, destroy));
    module->setFilename(path);
    LOG_INFO(g_logger) << "load module name=" << module->getName()
        << " version" << module->getVersion() << " path=" << module->getFilename()
        << " success";

    Config::LoadFromConfDir(Env::instance()->getConfigPath());
    return module;
}

__END__