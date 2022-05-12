#include <iostream>
#include "log.h"
#include "config.h"
#include "bytearray.h"
#include "http/tcp_server.h"
#include "iomanager.h"

using namespace std;
using namespace obelisk;
using namespace obelisk::http;

static Logger::ptr g_logger = LOG_SYSTEM();
int type = 0;

class EchoServer : public TcpServer{
public:
    EchoServer(int type) : m_type(type) {}
    void handle(Socket::ptr client);
private:
    int m_type;
};
void EchoServer::handle(Socket::ptr client){
    LOG_INFO(g_logger) << "handle client: " << *client;
    ByteArray::ptr ba(new ByteArray);
    while(true){
        ba->clear();
        vector<iovec> iovs;
        ba->getWriteBuffers(iovs, 1024);
        int rt = client->recv(&iovs[0], iovs.size());
        if(0 == rt){
            LOG_ERROR(g_logger) << "client error rt=" << rt << " errno=" << strerror(errno)
                << " errstr=" << strerror(errno);
            break;
        }
        ba->setPosition(ba->getPosition() + rt);
        ba->setPosition(0);
        if(0 == m_type){
            LOG_INFO(g_logger) << ba->toString();
        }else{
            LOG_INFO(g_logger) << ba->toHexString();
        }
    }
    LOG_INFO(g_logger) << "handle client end";
}


void run(){
    EchoServer::ptr es(new EchoServer(type));
    auto addr = Address::LookupAny("0.0.0.0:8020");
    while(!es->bind(addr)){
        sleep(2);
    }
    es->start();
}

int main(int argc, char **argv){
    YAML::Node root = YAML::LoadFile("/home/workspace/Obelisk/bin/conf/logs.yaml");
    Config::loadFromYaml(root);
    LOG_INFO(g_logger) << ">>>>>>>>>>>>>>>>>> load yaml end >>>>>>>>>>>>>";

    if(argc < 2){
        LOG_INFO(g_logger) << "used as[" << argv[0] << " -t] or [" << argv[0] << " -b]";
        return 0;
    }
    if(0 == strcmp(argv[1], "-b"))
        type = 1;
    IOManager iom(2);
    iom.schedule(run);
    return 0;
}