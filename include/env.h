#pragma once

#include <vector>
#include <map>
#include "mutex.h"

__OBELISK__

class Env{
public:
    bool init(int argc, char** argv);
    void add(const std::string& key, const std::string& value);
    bool has(const std::string& key);
    std::string get(const std::string& key, const std::string& default_value = "");
    void remove(const std::string& key);

    void addHelp(const std::string& key, const std::string& desc);
    void removeHelp(const std::string& key);
    void printHelp();

    const std::string& getExe() const { return m_exe; }
    const std::string& getCwd() const { return m_cwd; }
    
    bool setEnv(const std::string& key, const std::string& val);
    std::string getEnv(const std::string& key, const std::string& default_value = "");

    std::string getAbsolutePath(const std::string& path) const;
    std::string getConfigPath();

    static Env* instance(){
        static Env* env = new Env();
        return env;
    }
private:
    Env(){}
private:
    RWMutex m_mutex;
    std::map<std::string, std::string> m_args;
    std::vector<std::pair<std::string, std::string> > m_helps;

    std::string m_program;
    std::string m_exe;
    std::string m_cwd;
};
__END__