#include <iostream>
#include "log.h"
#include "config.h"
#include "iomanager.h"
#include "http/http_server.h"
#include "http/http_connection.h"

using namespace std;
using namespace obelisk;
using namespace obelisk::http;

static Logger::ptr g_logger = LOG_SYSTEM();

void test_pool(){
    HttpConnectionPool::ptr pool(new HttpConnectionPool("www.sylar.top", "", 80, 10, 1000 * 30, 5));
    IOManager::GetSelf()->addTimer(1000, [pool](){
        auto r = pool->doRequest(HttpMethod::GET, "/", 300);
        LOG_INFO(g_logger) << r->toString();
    }, true);
}

void run(){
    Address::ptr addr = Address::LookupAnyIPAddress("www.sylar.top:80");
    if(!addr){
        LOG_ERROR(g_logger) << "get addr error";
        return;
    }
    Socket::ptr sock = Socket::Create(addr->getFamily(), Socket::TCP);
    bool rt = sock->connect(addr);
    if(!rt){
        LOG_ERROR(g_logger) << "connect " << *addr << " failed";
        return;
    }
    HttpConnection::ptr conn(new HttpConnection(sock));
    HttpRequest::ptr req(new HttpRequest);
    req->setHeader("host", "www.sylar.top");
    LOG_INFO(g_logger) << "req: " << std::endl << *req;

    conn->sendRequest(req);
    auto rsp = conn->recvResponse();
    if(!rsp){
        LOG_ERROR(g_logger) << "recv response error";
        return;
    }
    LOG_INFO(g_logger) << "rsp: " << endl << *rsp;
    test_pool();
}
int main(int argc, char **argv){
    YAML::Node root = YAML::LoadFile("/home/workspace/Obelisk/bin/conf/logs.yaml");
    Config::loadFromYaml(root);
    LOG_INFO(g_logger) << ">>>>>>>>>>>>>>>>>> load yaml end >>>>>>>>>>>>>";
    IOManager iom(2);
    iom.schedule(run);
    return 0;
}