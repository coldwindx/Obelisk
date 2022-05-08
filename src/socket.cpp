#pragma once

#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include "macro.h"
#include "log.h"
#include "hook.h"
#include "fd_manager.h"
#include "socket.h"

__OBELISK__
static Logger::ptr g_logger = LOG_SYSTEM();

Socket::ptr Socket::Create(int family, int type){
    return Socket::ptr(new Socket(family, type, 0));
}

Socket::Socket(int family, int type, int protocol)
        : m_sock(-1), m_family(family), m_type(type), m_protocol(protocol), m_connected(false){
    
}
Socket::~Socket(){
    close();
}

int64_t Socket::getSendTimeout(){
    FdCtx::ptr ctx = FdManager::instance()->get(m_sock);
    if(ctx)
        return ctx->getTimeout(SO_SNDTIMEO);
    return -1;
}
void Socket::setSendTimeout(int64_t v){
    struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_SNDTIMEO, tv);
}

int64_t Socket::getRecvTimeout(){
    FdCtx::ptr ctx = FdManager::instance()->get(m_sock);
    if(ctx)
        return ctx->getTimeout(SO_RCVTIMEO);
    return -1;
}
void Socket::setRecvTimeout(int64_t v){
    struct timeval tv{int(v / 1000), int(v % 1000 * 1000)};
    setOption(SOL_SOCKET, SO_RCVTIMEO, tv);
}
// 获取socke句柄的相关信息
bool Socket::getOption(int level, int option, void* result, size_t* len){
    int rt = getsockopt(m_sock, level, option, result, (socklen_t*)len);
    if(rt){
        LOG_DEBUG(g_logger) << "getoption sock=" << m_sock << " level=" << level
            << " option=" << option << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}


bool Socket::setOption(int level, int option, const void* result, size_t len){
    if(setsockopt(m_sock, level, option, result, (socklen_t)len)){
        LOG_DEBUG(g_logger) << "getoption sock=" << m_sock << " level=" << level
            << " option=" << option << " errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}

Socket::ptr Socket::accept(){
    Socket::ptr sock(new Socket(m_family, m_type, m_protocol));
    int network = ::accept(m_sock, nullptr, nullptr);
    if(-1 == network){
        LOG_ERROR(g_logger) << "accept(" << m_sock << ") errno="
            << errno << "errstr=" << strerror(errno);
        return nullptr;
    }
    if(sock->init(network))
        return sock;
    return nullptr;
}
bool Socket::init(int sock){
    FdCtx::ptr ctx = FdManager::instance()->get(m_sock);
    if(ctx && ctx->isSocket() && !ctx->isClose()){
        m_sock = sock;
        m_connected = true;
        initSocket();
        getLocalAddress();
        getRemoteAddress();
        return true;
    }
    return false;
}
bool Socket::bind(const Address::ptr addr){
    if(!isVaild()){
        newSocket();    //TODO 创建失败
        if(UNLIKELY(!isVaild())){
            return false;
        }
    }
    if(addr->getFamily() != m_family){
        LOG_ERROR(g_logger) << "bind sock.family(" << m_family << ") addr.family("
            << addr->getFamily() << ") not equal, addr=" << addr->toString();
        return false;
    }
    if(::bind(m_sock, addr->getAddr(), addr->getAddrLen())){
        LOG_ERROR(g_logger) << "bind errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    getLocalAddress();
    return true;
}
bool Socket::connect(const Address::ptr addr, uint64_t timeout){
    if(!isVaild()){
        newSocket();    //TODO 创建失败
        if(UNLIKELY(!isVaild())){
            return false;
        }
    }
    if(addr->getFamily() != m_family){
        LOG_ERROR(g_logger) << "connect sock.family(" << m_family << ") addr.family("
            << addr->getFamily() << ") not equal, addr=" << addr->toString();
        return false;
    }
    if((uint64_t)-1 == timeout){
        if(::connect(m_sock, addr->getAddr(), addr->getAddrLen())){
            LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                << ") error errno=" << errno << " errstr=" << strerror(errno);
            close();
            return false;
        }
    }else{
        if(::connect_with_timeout(m_sock, addr->getAddr(), addr->getAddrLen(), timeout)){
            LOG_ERROR(g_logger) << "sock=" << m_sock << " connect(" << addr->toString()
                << ") timeout=" << timeout << " error errno=" << errno << " errstr=" << strerror(errno);
            close();
            return false;
        }
    }
    m_connected = true;
    getRemoteAddress();
    getLocalAddress();
    return true;
}
bool Socket::listen(int backlog){
    if(!isVaild()){
        LOG_ERROR(g_logger) << "listen error sock=-1";
        return false;
    }
    if(::listen(m_sock, backlog)){
        LOG_ERROR(g_logger) << "listen error errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    return true;
}
bool Socket::close(){
    if(!m_connected && -1 == m_sock)
        return true;
    m_connected = false;
    if(-1 != m_sock){
        ::close(m_sock);
        m_sock = -1;
    }
    return false;
}

int Socket::send(const void* buffer, size_t length, int flags){
    if(isConnected()){
        return ::send(m_sock, buffer, length, flags);
    }
    return -1;
}
int Socket::send(const iovec* buffers, size_t length, int flags){
    if(isConnected()){
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}
int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags){
    if(isConnected()){
        return ::sendto(m_sock, buffer, length, flags, to->getAddr(), to->getAddrLen());
    }
    return -1;
}
int Socket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags){
    if(isConnected()){
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = to->getAddr();
        msg.msg_namelen = to->getAddrLen();
        return ::sendmsg(m_sock, &msg, flags);
    }
    return -1;
}

int Socket::recv(void* buffer, size_t length, int flags){
    if(isConnected()){
        return ::recv(m_sock, buffer, length, flags);
    }
    return -1;
}
int Socket::recv(iovec* buffers, size_t length, int flags){
    if(isConnected()){
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}
int Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags){
    if(isConnected()){
        socklen_t len = from->getAddrLen();
        return ::recvfrom(m_sock, buffer, length, flags, from->getAddr(), &len);
    }
    return -1;
}
int Socket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags){
    if(isConnected()){
        msghdr msg;
        memset(&msg, 0, sizeof(msg));
        msg.msg_iov = (iovec*)buffers;
        msg.msg_iovlen = length;
        msg.msg_name = from->getAddr();
        msg.msg_namelen = from->getAddrLen();
        return ::recvmsg(m_sock, &msg, flags);
    }
    return -1;
}

Address::ptr Socket::getRemoteAddress(){
    if(m_remoteAddress)
        return m_remoteAddress;
    Address::ptr result;
    switch(m_family){
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknowAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    if(getpeername(m_sock, result->getAddr(), &addrlen)){
        LOG_ERROR(g_logger) << "getpeername error sock=" << m_sock
            << " errno=" << errno << " errstr=" << strerror(errno);
        return Address::ptr(new UnknowAddress(m_family));
    }
    if(AF_UNIX == m_family){
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    return m_remoteAddress = result;
}
Address::ptr Socket::getLocalAddress(){
    if(m_localAddress)
        return m_localAddress;
    Address::ptr result;
    switch(m_family){
        case AF_INET:
            result.reset(new IPv4Address());
            break;
        case AF_INET6:
            result.reset(new IPv6Address());
            break;
        case AF_UNIX:
            result.reset(new UnixAddress());
            break;
        default:
            result.reset(new UnknowAddress(m_family));
            break;
    }
    socklen_t addrlen = result->getAddrLen();
    if(getsockname(m_sock, result->getAddr(), &addrlen)){
        LOG_ERROR(g_logger) << "getsockname error sock=" << m_sock
            << " errno=" << errno << " errstr=" << strerror(errno);
        return Address::ptr(new UnknowAddress(m_family));
    }
    if(AF_UNIX == m_family){
        UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
        addr->setAddrLen(addrlen);
    }
    return m_localAddress = result;
}

bool Socket::isVaild() const{
    return -1 != m_sock;
}
int Socket::getError(){
    int error = 0;
    size_t len = sizeof(error);
    if(!getOption(SOL_SOCKET, SO_ERROR, &error, &len))
        return -1;
    return error;
}

std::ostream& Socket::dump(std::ostream& os) const{
    os << "[Socket sock=" << m_sock << " is_connected=" << m_connected
        << " family=" << m_family << " type=" << m_type << " protocol=" << m_protocol;
    if(m_localAddress)
        os << " local_address=" << m_localAddress->toString();
    if(m_remoteAddress)
        os << "remote_address=" << m_remoteAddress->toString();
    return os << "]";
}


bool Socket::cancelRead(){
    return IOManager::GetSelf()->cancelEvent(m_sock, IOManager::READ);
}
bool Socket::cancelWrite(){
    return IOManager::GetSelf()->cancelEvent(m_sock, IOManager::WRITE);
}
bool Socket::cancelAccept(){
    return IOManager::GetSelf()->cancelEvent(m_sock, IOManager::READ);
}
bool Socket::cancelAll(){
    return IOManager::GetSelf()->cancelAll(m_sock);
}

void Socket::initSocket(){
    int val = 1;
    setOption(SOL_SOCKET, SO_REUSEADDR,val);
    if(SOCK_STREAM == m_type)
        setOption(IPPROTO_TCP, TCP_NODELAY, val);
}
void Socket::newSocket(){
    m_sock = socket(m_family, m_type, m_protocol);
    if(LIKELY(-1 != m_sock)){
        initSocket();
    }else{
        LOG_ERROR(g_logger) << "socket(" << m_family << ", "
            << m_type << ", " << m_protocol << ") errno=" << errno
            << " errstr=" << strerror(errno);
    }
}

__END__