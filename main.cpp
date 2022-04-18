#include <iostream>
#include <string>
#include "log/log.h"
#include "config/config.h"

using namespace std;
using namespace obelisk;

ConfigVar<list<int> >::ptr g_value_config = Config::lookup("logs", list<int>{1,2,4,2}, "system vector");


int main(){
    cout << "old_value:" << g_value_config->toString() << endl;
    YAML::Node root = YAML::LoadFile("/home/workspace/Obelisk/bin/conf/logs.yaml");
    Config::loadFromYaml(root);
    cout << "new_value" << g_value_config->toString() << endl;
    return 0;
}