#include <unistd.h>
#include "env.h"
#include "log.h"
#include "config.h"
#include "daemon.h"
#include "utils.h"
#include "application.h"
#include "http/http_server.h"

__OBELISK__

static Logger::ptr g_logger = LOG_SYSTEM();

struct HttpServerConf{
    std::vector<std::string> address;
    int keepalive = 0;
    int timeout = 1000 * 2 * 60;
    std::string name = "";
    bool isValid() const{
        return !address.empty();
    }
    bool operator==(const HttpServerConf& oth) const{
        return address == oth.address && keepalive == oth.keepalive
                && timeout == oth.timeout && name == oth.name;
    }
};


template<>
class Convert<std::string, HttpServerConf> {
public:
	HttpServerConf operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        HttpServerConf conf;
        conf.keepalive = node["keepalive"].as<int>(conf.keepalive);
        conf.timeout = node["timeout"].as<int>(conf.timeout);
        conf.name = node["name"].as<std::string>(conf.name);

        if(node["address"].IsDefined())
            for(size_t i = 0; i < node["address"].size(); ++i)
                conf.address.push_back(node["address"][i].as<std::string>());

        return conf;
    }
};

template<>
class Convert<HttpServerConf, std::string> {
public:
	std::string operator()(const HttpServerConf& conf) {
        YAML::Node node;
        node["name"] = conf.name;
        node["keepalive"] = conf.keepalive;
        node["timeout"] = conf.timeout;
        for(auto & i : conf.address)
            node["address"].push_back(i);
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};


static ConfigVar<std::string>::ptr g_server_work_path = 
        Config::Lookup("server.work_path", std::string("/apps/work/obelisk"), "server pid file");
static ConfigVar<std::string>::ptr g_server_pid_file =
        Config::Lookup("server.pid_file", std::string("obelisk.pid"), "server pid file");
static ConfigVar<std::vector<HttpServerConf> >::ptr g_http_servers_conf =
        Config::Lookup("http_servers", std::vector<HttpServerConf>(), "http servers");



Application* Application::s_instance = nullptr;

Application::Application(){
    s_instance = this;
}

bool Application::init(int argc, char** argv){
    m_argc = argc;
    m_argv = argv;

    Env::instance()->addHelp("s", "start with the terminal");
    Env::instance()->addHelp("d", "run as daemon");
    Env::instance()->addHelp("c", "conf path default: ./conf");
    Env::instance()->addHelp("h", "print help");

    if(!Env::instance()->init(argc, argv)){
        Env::instance()->printHelp();
        return false;
    }

    if(Env::instance()->has("h")){
        Env::instance()->printHelp();
        return false;
    }

    int run_type = 0;
    if(Env::instance()->has("s"))
        run_type = 1;
    if(Env::instance()->has("d"))
        run_type = 2;
    if(0 == run_type){
        Env::instance()->printHelp();
        return false;
    }

    std::string pidfile = g_server_work_path->getValue()
            + "/" + g_server_pid_file->getValue();
    if(FileUtils::IsRunningPidfile(pidfile)){
        LOG_ERROR(g_logger) << "server is running:" << pidfile;
        return false;
    }

    std::string path = Env::instance()->get("c", "../bin/conf");
    std::string conf_path = Env::instance()->getAbsolutePath(path);
    LOG_INFO(g_logger) << "load conf path:" << conf_path;
    Config::LoadFromConfDir(conf_path);

    if(!FileUtils::Mkdir(g_server_work_path->getValue())){
        LOG_FATAL(g_logger) << "create work path [" << g_server_work_path->getValue()
            << "] errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    return true;
}
bool Application::run(){
    bool is_daemon = Env::instance()->has("d");
    return start_daemon(m_argc, m_argv
                        , std::bind(&Application::main, this, std::placeholders::_1, std::placeholders::_2)
                        , is_daemon);
}

int Application::main(int argc, char** argv){
    std::string pidfile = g_server_work_path->getValue()
            + "/" + g_server_pid_file->getValue();
    std::ofstream ofs(pidfile);
    if(!ofs){
        LOG_ERROR(g_logger) << "open pidfile " << pidfile << " failed";
        return false;
    }
    ofs << getpid();

    IOManager iom(1);
    iom.schedule(std::bind(&Application::runCoroutine, this));
    iom.stop();
    return 0;
}

int Application::runCoroutine(){
    auto http_confs = g_http_servers_conf->getValue();
    for(auto & i : http_confs){
        LOG_INFO(g_logger) << Convert<HttpServerConf, std::string>()(i);
        std::vector<Address::ptr> addresses;
        for(auto & a : i.address){
            size_t pos = a.find(":");
            if(std::string::npos == pos){
                LOG_ERROR(g_logger) << "invalid address: " << a;
                continue;
            }
            auto addr = Address::LookupAny(a);
            if(addr){
                addresses.push_back(addr);
                continue;
            }
            std::vector<std::pair<Address::ptr, uint32_t> > result;
            bool b = Address::GetInterfaceAddresses(result, a.substr(0, pos));
            if(!b){
                LOG_ERROR(g_logger) << "invalid address: " << a;
                continue;
            }
            for(auto & x : result){
                auto ipaddr = std::dynamic_pointer_cast<IPAddress>(x.first);
                if(ipaddr){
                    ipaddr->setPort(atoi(a.substr(pos + 1).c_str()));
                }
                addresses.push_back(ipaddr);
            }
        }
        http::HttpServer::ptr server(new http::HttpServer((i.keepalive)));
        std::vector<Address::ptr> fails;
        if(!server->bind(addresses, fails)){
            for(auto & x : fails){
                LOG_ERROR(g_logger) << "bind address fail: " << *x;
            }
            _exit(0);
        }
        if(!i.name.empty())
            server->setName(i.name);
        server->start();
        m_servers.push_back(server);
    }
    return 0;
}
__END__