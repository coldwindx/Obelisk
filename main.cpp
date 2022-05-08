#include "coroutine/iomanager.h"
#include "log.h"
#include "config/config.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include "iomanager.h"
#include "hook.h"
#include "address.h"
#include "socket.h"

using namespace std;
using namespace obelisk;
Logger::ptr g_logger = LOG_SYSTEM();

void test_coroutine(){
    LOG_INFO(g_logger) << "test_coroutine";
}

void test1(){
    IOManager iom(2, "IOManager");
    iom.schedule(&test_coroutine);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "110.242.68.4", &addr.sin_addr.s_addr);

    int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    iom.addEvent(sock, IOManager::READ, [](){
        LOG_INFO(g_logger) << "read callback";
    });
    
    iom.addEvent(sock, IOManager::WRITE, [&](){
        LOG_INFO(g_logger) << "write callback";
        IOManager::GetSelf()->cancelEvent(sock, IOManager::READ);
    });
}


void test_hook(){
    LOG_INFO(g_logger) << "test_hook begin";
    IOManager manager(2);
    manager.schedule([](){
        sleep(2);
        LOG_INFO(g_logger) << "sleep 2s";
    });
    manager.schedule([](){
        sleep(3);
        LOG_INFO(g_logger) << "sleep 3s";
    });
    LOG_INFO(g_logger) << "test_hook end";
}

void test_timer(){
    IOManager iom(2);
    Timer::ptr timer = iom.addTimer(500, [&timer](){
        LOG_INFO(g_logger) << "hello,timer!";
        static int i = 0;
        if(++i == 5)
            timer->cancel();
    }, true);
}

void test_socket(){
    LOG_INFO(g_logger) << "test hook begin";
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "110.242.68.3", &addr.sin_addr.s_addr);
    LOG_INFO(g_logger) << "start to connect";
    int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    LOG_INFO(g_logger) << "connect rt=" << rt << " errno=" << errno;
    if(rt){
        return;
    }
    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0);
    LOG_INFO(g_logger) << "send rt=" << rt << ", errno=" << errno;
    if(rt <= 0)
        return;
    string buf;
    buf.resize(4096);
    rt = recv(sock, &buf[0], buf.size(), 0);
    LOG_INFO(g_logger) << "recv rt=" << rt << ", errno" << errno;
    if(rt <= 0)
        return;
    buf.resize(rt);
    LOG_INFO(g_logger) << buf;

}

void test_address(){
    LOG_INFO(g_logger) << "----------IPV4-----------";
    std::vector<Address::ptr> addrs;
    bool v = Address::Lookup(addrs, "www.baidu.com");
    if(!v){
        LOG_ERROR(g_logger) << "lookup fail";
        return ;
    }
    for(size_t i = 0; i < addrs.size(); ++i){
        LOG_INFO(g_logger) << i << " - " << addrs[i]->toString();
    }

    auto addr = IPAddress::Create("127.0.0.8");
    if(addr){
        LOG_ERROR(g_logger) << addr->toString();
    }
    LOG_INFO(g_logger) << "----------IPV6-----------";
    std::multimap<std::string, std::pair<Address::ptr, uint32_t> > results;
    bool v2 = Address::GetInterfaceAddresses(results);
    if(!v2){
        LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
        return;
    }
    for(auto & i : results){
        LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString()
            << " - " << i.second.second;
    }
    LOG_INFO(g_logger) << "----------END-----------";
}

void test_socket2(){
    IPAddress::ptr addr = Address::LookupAnyIPAddress("www.baidu.com");
    if(addr)
        LOG_INFO(g_logger) << "get address: " << addr->toString();
    else{
        LOG_ERROR(g_logger) << "get address fail";
        return;
    }
    Socket::ptr sock = Socket::Create(addr->getFamily());
    addr->setPort(80);
    if(!sock->connect(addr)){
        LOG_ERROR(g_logger) << "connect " << addr->toString() << " fail";
        return;
    }
    else{
        LOG_INFO(g_logger) << "connect " << addr->toString() << " connected";
    }
    const char buff[] = "GET / HTTP/1.0\r\n\r\n";
    int rt = sock->send(buff, sizeof(buff));
    if(rt <= 0){
        LOG_ERROR(g_logger) << "send fail rt=" << rt;
        return;
    }
    string buffs;
    buffs.resize(4096);
    rt = sock->recv(&buffs[0], buffs.size());
    if(rt <= 0){
        LOG_ERROR(g_logger) << "recv fail rt=" << rt;
        return;
    }
    buffs.resize(rt);
    LOG_INFO(g_logger) << buffs;
}
int main(){
    YAML::Node root = YAML::LoadFile("/home/workspace/Obelisk/bin/conf/logs.yaml");
    Config::loadFromYaml(root);
    LOG_INFO(g_logger) << ">>>>>>>>>>>>>>>>>> load yaml end >>>>>>>>>>>>>";
    IOManager iom;
    // test1();
    // test_timer();
    // test_hook();
    // test_socket();
    // test_address();
    // iom.schedule(&test_socket2);
    return 0;
}