#include <iostream>
#include <string>
#include "log/log.h"
#include "config/config.h"
#include "log/log_config.h"

using namespace std;
using namespace obelisk;

void test_config() {
	//cout << LogManager::instance()->toYamlString() << endl;
	YAML::Node root = YAML::LoadFile("/home/workspace/bin/con/logs.yaml");
	cout << "====================================" << endl;
	Config::loadFromYaml(root);
	//cout << LogManager::instance()->toYamlString() << endl;
	LOG_INFO(LOG_NAME("system")) << "hello, system!";
}

void test(){
    //cout << Convert<std::set<LogConfig>, string>()(set<LogConfig>()) << endl;
    //ConfigVar<std::set<LogConfig> >::ptr g_log_defines = Config::lookup("logs", std::set<LogConfig>(), "logs config");
    YAML::Node root = YAML::LoadFile("/home/workspace/Obelisk/bin/conf/logs.yaml");
    Config::loadFromYaml(root);
    LoggerManager::ptr manager = LoggerManager::instance();
    LOG_INFO(LOG_NAME("root")) << "hello, system!";
}
int main()
{
	//test_config();
    test();
	return 0;
}