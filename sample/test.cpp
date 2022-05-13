#include "coroutine/iomanager.h"
#include "log.h"
#include "coroutine/coroutine_macro.h"
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
#include "bytearray.h"
#include "http/http.h"
#include "http/http_parser.h"

using namespace std;
using namespace obelisk;
using namespace obelisk::http;
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
void test_bytearray(){
#define XX(type, len, write_fun, read_fun, base_len) { \
    vector<type> vec;  \
    for(int i = 0; i < len; ++i){  \
        vec.push_back(rand());  \
    }  \
    ByteArray::ptr ba(new ByteArray(base_len));  \
    for(auto& i : vec){  \
        ba->write_fun(i);  \
    }  \
    ba->setPosition(0);  \
    for(size_t i = 0; i < vec.size(); ++i){  \
        int32_t v = ba->read_fun();  \
        OBELISK_ASSERT(v == vec[i]);  \
    }  \
    OBELISK_ASSERT(ba->getReadSize() == 0); \
    LOG_INFO(g_logger) << #write_fun "/" #read_fun " (" #type ") len=" << len \
        << " base_len=" << base_len << " size=" << ba->getSize();   \
    }
 
    XX(int8_t, 100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t, 100, writeFint16, readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t, 100, writeFint32, readFint32, 1);
    XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(int64_t, 100, writeFint64, readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t, 100, writeInt32, readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t, 100, writeInt64, readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);

#undef XX

#define XX(type, len, write_fun, read_fun, base_len) { \
    vector<type> vec;  \
    for(int i = 0; i < len; ++i){  \
        vec.push_back(rand());  \
    }  \
    ByteArray::ptr ba(new ByteArray(base_len));  \
    for(auto& i : vec){  \
        ba->write_fun(i);  \
    }  \
    ba->setPosition(0);  \
    for(size_t i = 0; i < vec.size(); ++i){  \
        int32_t v = ba->read_fun();  \
        OBELISK_ASSERT(v == vec[i]);  \
    }  \
    OBELISK_ASSERT(ba->getReadSize() == 0); \
    LOG_INFO(g_logger) << #write_fun "/" #read_fun " (" #type ") len=" << len \
        << " base_len=" << base_len << " size=" << ba->getSize();   \
    ba->setPosition(0); \
    OBELISK_ASSERT(ba->writeToFile("/tmp/" #type "_" #len "_" #read_fun ".dat")); \
    ByteArray::ptr ba2(new ByteArray(base_len * 2));    \
    OBELISK_ASSERT(ba2->readFromFile("/tmp/" #type "_" #len "_" #read_fun ".dat"));   \
    ba2->setPosition(0);    \
    OBELISK_ASSERT(ba->toString() == ba2->toString());  \
    OBELISK_ASSERT(ba->getPosition() == 0); \
    OBELISK_ASSERT(ba2->getPosition() == 0);    \
    }

    XX(int8_t, 100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t, 100, writeFint16, readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t, 100, writeFint32, readFint32, 1);
    XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(int64_t, 100, writeFint64, readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t, 100, writeInt32, readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t, 100, writeInt64, readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);
#undef XX

}

void test_http(){
    LOG_INFO(g_logger) << ">>>>>>>>>>>>>>>>>> test http request >>>>>>>>>>>>>";
    HttpRequest::ptr req(new HttpRequest);
    req->setHeader("host", "www.sylar.top");
    req->setBody("hello, sylar");
    req->dump(std::cout) << std::endl;
    LOG_INFO(g_logger) << ">>>>>>>>>>>>>>>>>> test http response >>>>>>>>>>>>>";
    HttpResponse::ptr rsp(new HttpResponse);
    rsp->setHeader("X-X", "Sylar");
    rsp->setBody("hello, sylar");
    rsp->dump(std::cout) << std::endl;
}
char request_data[] = "POST / HTTP/1.1\r\n"
    "Host: www.sylar.top\r\n"
    "Content-Length: 10\r\n\r\n"
    "1234567890";
const char response_data[] = "HTTP/1.1 200 OK\r\n"
        "Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
        "Server: Apache\r\n"
        "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
        "ETag: \"51-47cf7e6ee8400\"\r\n"
        "Accept-Ranges: bytes\r\n"
        "Content-Length: 81\r\n"
        "Cache-Control: max-age=86400\r\n"
        "Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
        "Connection: Close\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html>\r\n"
        "<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
        "</html>\r\n";
void test_http_parser(){
    LOG_INFO(g_logger) << ">>>>>>>>>>>>>>>>>> test http request parser >>>>>>>>>>>>>";
    {
        HttpRequestParser parser;
        string tmp = request_data;
        size_t s = parser.execute(request_data, tmp.size()); 
        LOG_INFO(g_logger) << "execute rt=" << s << " has_error=" << parser.hasError()
            << " is_finished=" << parser.isFinished()
            << " total=" << tmp.size()
            << " content_length=" << parser.getContentLength();
        tmp.resize(tmp.size() - s);
        LOG_INFO(g_logger) << parser.getData()->toString();
    }
    LOG_INFO(g_logger) << ">>>>>>>>>>>>>>>>>> test http response parser >>>>>>>>>>>>>";
    {
        HttpResponseParser parser;
        string tmp = response_data;
        size_t s = parser.execute(&tmp[0], tmp.size()); 
        LOG_INFO(g_logger) << "execute rt=" << s << " has_error=" << parser.hasError()
            << " is_finished=" << parser.isFinished()
            << " total=" << tmp.size()
            << " content_length=" << parser.getContentLength();
        tmp.resize(tmp.size() - s);
        LOG_INFO(g_logger) << parser.getData()->toString();
        LOG_INFO(g_logger) << tmp;
    }

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
    // test_bytearray();
    // test_http();
    test_http_parser();
    return 0;
}