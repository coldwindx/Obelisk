#include <iostream>
#include "log.h"
#include "config.h"
#include "http/http_server.h"

using namespace std;
using namespace obelisk;
using namespace obelisk::http;

static Logger::ptr g_logger = LOG_SYSTEM();

void run(){
    HttpServer::ptr server(new HttpServer());
    Address::ptr addr = Address::LookupAnyIPAddress("0.0.0.0:8020");
    while(!server->bind(addr)){
        sleep(2);
    }
    auto sd = server->getServletDispatch();
    sd->addServlet("/obelisk/test", [](HttpRequest::ptr req, HttpResponse::ptr rsp, HttpSession::ptr session){
        rsp->setBody(req->toString());
        return 0;
    });
    sd->addGlobServlet("/obelisk/*", [](HttpRequest::ptr req, HttpResponse::ptr rsp, HttpSession::ptr session){
        rsp->setBody("Glob:\r\n" + req->toString());
        return 0;
    });
    server->start();
}
int main(int argc, char **argv){
    YAML::Node root = YAML::LoadFile("/home/workspace/Obelisk/bin/conf/logs.yaml");
    Config::loadFromYaml(root);
    LOG_INFO(g_logger) << ">>>>>>>>>>>>>>>>>> load yaml end >>>>>>>>>>>>>";
    IOManager iom(2);
    iom.schedule(run);
    return 0;
}