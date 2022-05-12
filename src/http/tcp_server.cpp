#include "log.h"
#include "config.h"
#include "http/tcp_server.h"

__OBELISK__
__HTTP__

static ConfigVar<uint64_t>::ptr g_tcp_server_recv_timeout 
            = Config::lookup("tcp_server.recv_timeout", (uint64_t)(60 * 1000 * 2), "tcp server recv timeout");
static Logger::ptr g_logger = LOG_SYSTEM();

TcpServer::TcpServer(IOManager *worker, IOManager *aworker) 
        : m_worker(worker)
        , m_acceptWorker(aworker)
        , m_recvTimeout(g_tcp_server_recv_timeout->getValue())
        , m_name("obelisk/1.0.0")
        , m_stop(true) {

}
TcpServer::~TcpServer(){
    for(auto & sock : m_socks)
     sock->close();
    m_socks.clear();
}
bool TcpServer::bind(Address::ptr addr){
    Socket::ptr sock = Socket::Create();
    if(!sock->bind(addr)){
        LOG_ERROR(g_logger) << "bind fail errno=" << errno
            << " errstr=" << strerror(errno) << " addr=[" << addr->toString() << "]";
        m_socks.clear();
        return false;
    }
    if(!sock->listen()){
        LOG_ERROR(g_logger) << "listen fail errno=" << errno
            << " errstr=" << strerror(errno) << " addr=[" << addr->toString() << "]";
        m_socks.clear();
        return false;
    }
    m_socks.push_back(sock);
    return true;
}
bool TcpServer::bind(const std::vector<Address::ptr>& addrs, std::vector<Address::ptr>& fails){
    for(auto & addr : addrs){
        Socket::ptr sock = Socket::Create(addr->getFamily());
        if(!sock->bind(addr)){
            LOG_ERROR(g_logger) << "bind fail errno=" << errno
                << " errstr=" << strerror(errno) << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        if(!sock->listen()){
            LOG_ERROR(g_logger) << "listen fail errno=" << errno
                << " errstr=" << strerror(errno) << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        m_socks.push_back(sock);
    }
    if(!fails.empty()){
        m_socks.clear();
        return false;
    }
    for(auto & i : m_socks){
        LOG_INFO(g_logger) << "server bind success: " << i;
    }
    return true;
}
void TcpServer::start(){
    if(!m_stop) return;
    m_stop = false;
    for(auto & sock : m_socks)
        m_acceptWorker->schedule(std::bind(&TcpServer::accept, shared_from_this(), sock));
}   
void TcpServer::stop(){
    m_stop = true;
    // 防止schedule异步处理时TcpServer引用计数归0
    auto self = shared_from_this();
    m_acceptWorker->schedule([this, self](){
        for(auto & sock : m_socks){
            sock->cancelAll();
            sock->close();
        }
        m_socks.clear();
    });
}
void TcpServer::handle(Socket::ptr client){
    LOG_DEBUG(g_logger) << "handle client: " << *client; 
}
void TcpServer::accept(Socket::ptr sock){
    while(!m_stop){
        Socket::ptr client = sock->accept();
        if(client){
            client->setRecvTimeout(m_recvTimeout);
            m_worker->schedule(std::bind(&TcpServer::handle, shared_from_this(), client));
        }else
            LOG_ERROR(g_logger) << "accept errno=" << errno << " errstr=" << strerror(errno);
    }
}

__END__
__END__