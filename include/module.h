#pragma once

#include <unordered_map>
#include "mutex.h"
#include "stream.h"

__OBELISK__

class Module{
public:
    typedef std::shared_ptr<Module> ptr;

    Module(const std::string& name, const std::string& version, const std::string& filename);
    ~Module() {}

    std::string getName() const { return m_name; }
    std::string getVersion() const { return m_version; }
    std::string getFilename() const { return m_filename; }
    std::string getId() const { return m_id; }

    void setFilename(const std::string& v) { m_filename = v; }

    virtual bool onLoad();
    virtual bool onUnload();

    virtual bool onConnect(Stream::ptr stream);
    virtual bool onDisconnect(Stream::ptr stream);

    virtual bool onServerReady();
    virtual bool onServerUp();

    virtual bool beforeArgsParse(int argc, char** argv);
    virtual bool afterArgsParse(int argc, char** argv);
    
private:
    std::string m_name;
    std::string m_version;
    std::string m_filename;
    std::string m_id;
};

class ModuleManager{
public:
    typedef std::shared_ptr<ModuleManager> ptr;
    static ModuleManager* instance();

    void init();
    void add(Module::ptr module);
    void del(const std::string& name);
    void clear();
    Module::ptr get(const std::string& name);
    void list(std::vector<Module::ptr>& modules);
private:
    ModuleManager() {}
private:
    RWMutex m_mutex;
    std::unordered_map<std::string, Module::ptr> m_modules;

};
__END__