#include <iostream>
#include "log.h"
#include "config.h"
#include "address.h"
#include "http/http_server.h"
#include "http/http_connection.h"
#include "http/uri.h"

using namespace std;
using namespace obelisk;
using namespace obelisk::http;

static Logger::ptr g_logger = LOG_SYSTEM();

int main(int argc, char **argv){
    YAML::Node root = YAML::LoadFile("/home/workspace/Obelisk/bin/conf/logs.yaml");
    Config::loadFromYaml(root);
    LOG_INFO(g_logger) << ">>>>>>>>>>>>>>>>>> load yaml end >>>>>>>>>>>>>";
    Uri::ptr uri = Uri::Create("http://www.sylar.top/test/中/uri?id=100&name=sylar#frg");
    cout << *uri << endl;
    auto addr = uri->createAddress();
    cout << *addr << endl;
    return 0;
}