#include <vector>
#include <sstream>
#include "log.h"
#include "http/http_parser.h"
#include "http/http_connection.h"

__OBELISK__
__HTTP__

static Logger::ptr g_logger = LOG_SYSTEM();

std::string HttpResult::toString() const{
    std::stringstream ss;
    ss << "[HttpResult result=" << result
        << " error=" << error
        << " response=" << (response ? response->toString() : "nullptr")
        << "]";
    return ss.str(); 
}

HttpConnection::HttpConnection(Socket::ptr sock, bool owner)
        : SocketStream(sock, owner) {

}
HttpResponse::ptr HttpConnection::recvResponse(){
    HttpResponseParser::ptr parser(new HttpResponseParser);
    uint64_t buff_size = HttpResponseParser::GetHttpResponseBufferSize();
    std::shared_ptr<char> buffer(new char[buff_size + 1], std::default_delete<char[]>());

    char *data = buffer.get();
    int offset = 0;
    do{
        int len = read(data + offset, buff_size - offset);
        if(len <= 0) {
            close();
            return nullptr;
        }
        len += offset;
        data[len] = '\0';
        size_t nparse = parser->execute(data, len + offset, false);
        if(parser->hasError()) {
            close();
            return nullptr;
        }
        offset = len - nparse;
        if(offset == buff_size) {
            close();
            return nullptr;
        }
        if(parser->isFinished()) break;
    } while(true);

    auto& client_parser = parser->getParser();
    if(client_parser.chunked){
        std::string body;
        int len = offset;

        do{
            do{
                int rt = read(data + len, buff_size - len);
                if(rt <= 0){
                    close();
                    return nullptr;
                }
                len += rt;
                data[len] = '\0';
                size_t nparse = parser->execute(data, len, true);
                if(parser->hasError()){
                    close();
                    return nullptr;
                }
                len -= nparse;
                if(len == (int)buff_size){
                    close();
                    return nullptr;
                }
                
            }while(!parser->isFinished());

            len -= 2;

            if(client_parser.content_len <= len){
                body.append(data, client_parser.content_len);
                memmove(data, data + client_parser.content_len, len - client_parser.content_len);
                len -= client_parser.content_len;
            }else{
                body.append(data, len);
                int left = client_parser.content_len - len;
                while(0 < left){
                    int rt = read(data, buff_size < left ? buff_size : left);
                    if(rt <= 0){
                        close();
                        return nullptr;
                    }
                    body.append(data, rt);
                    left -= rt;
                }
                len = 0;
            }
        }while(client_parser.chunks_done);
        parser->getData()->setBody(body);
    }else{
        int64_t length = parser->getContentLength();
        if(0 < length){
            std::string body;
            body.resize(length);
            int len = 0;
            if(offset <= length){
                memcpy(&body[0], data, offset);
                len = offset;
            }else{
                memcpy(&body[0], data, length);
                len = length;
            }
            length -= offset;

            if(0 < length){
                if(readFixSize(&body[body.size()], length) <= 0) {
                    close();
                    return nullptr;
                }
            }
            parser->getData()->setBody(body);
        }
    }

    return parser->getData();
}

int HttpConnection::sendRequest(HttpRequest::ptr request){
    std::stringstream ss;
    ss << *request;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}
HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
                                        , const std::string& url
                                        , uint64_t timeout
                                        , const std::map<std::string, std::string>& headers
                                        , const std::string& body){
    Uri::ptr uri = Uri::Create(url);
    if(!uri)
        return std::make_shared<HttpResult>((int)HttpResult::Error::INvALID_URL
                                            , nullptr, "invalid url: " + url);
    return DoRequest(method, uri, timeout, headers, body);                                        
}
HttpResult::ptr HttpConnection::DoRequest(HttpMethod method
                                        , Uri::ptr uri
                                        , uint64_t timeout
                                        , const std::map<std::string, std::string>& headers
                                        , const std::string& body){
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(uri->getPath());
    req->setQuery(uri->getQuery());
    req->setFragment(uri->getFragment());
    req->setMethod(method);
    bool has_host = false;
    for(auto & i : headers){
        if(0 == strcasecmp(i.first.c_str(), "connection")){
            if(0 == strcasecmp(i.second.c_str(), "keep-alive"))
                req->setClose(false);
            continue;
        }
        if(0 == strcasecmp(i.first.c_str(), "host"))
            has_host = !i.second.empty();
        req->setHeader(i.first, i.second);
    }
    if(!has_host)
        req->setHeader("Host", uri->getHost());
    req->setBody(body);
    return DoRequest(req, uri, timeout);
}
HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req
                                        , Uri::ptr uri
                                        , uint64_t timeout){
    Address::ptr addr = uri->createAddress();
    if(!addr)
        return std::make_shared<HttpResult>((int)HttpResult::Error::INvALID_HOST
                        , nullptr, "invalid host: " + uri->getHost());
    Socket::ptr sock = Socket::Create(addr->getAddrLen(), Socket::TCP);
    if(!sock) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::CREATE_SOCKET_ERROR
                , nullptr, "create socket fail: " + addr->toString()
                        + " errno=" + std::to_string(errno)
                        + " errstr=" + std::string(strerror(errno)));
    }
    if(!sock->connect(addr)) {
        return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECT_FAIL
                , nullptr, "connect fail: " + addr->toString());
    }  
    sock->setRecvTimeout(timeout);
    HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);
    int rt = conn->sendRequest(req);
    if(0 == rt)
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER
                        , nullptr, "send request close by peer: " + addr->toString()); 
    if(rt < 0)
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR
                        , nullptr, "send request socket error errno: " + std::to_string(errno)
                        + " errstr=" + strerror(errno));
    auto rsp = conn->recvResponse();
    if(!rsp)
        return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT
                        , nullptr, "recv response timeout: " + addr->toString()
                         + " timeout=" + std::to_string(timeout)); 

    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "success"); 
}

HttpConnectionPool::HttpConnectionPool(const std::string& host
                        , const std::string& vhost
                        , uint32_t port
                        , uint32_t maxSize
                        , uint32_t maxAliveTime
                        , uint32_t maxRequestSize)
        : m_host(host), m_vhost(vhost), m_port(port), m_maxSize(maxSize)
        , m_maxAliveTime(maxAliveTime), m_maxRequestSize(maxRequestSize){
    
}

HttpConnection::ptr HttpConnectionPool::getConnection(){
    uint64_t now = GetCurrentMS();
    std::vector<HttpConnection*> invalid_conns;
    HttpConnection* ptr = nullptr;
    Lock lock(m_mutex);
    while(!m_conns.empty()){
        auto conn = *m_conns.begin();
        m_conns.pop_front();
        if(!conn->isConnected() 
                    || now < conn->m_createTime + m_maxAliveTime){
            invalid_conns.push_back(conn);
            continue;
        }
        ptr = conn;
        break;
    }
    lock.unlock();
    for(auto & i : invalid_conns) delete i;
    m_total -= invalid_conns.size();
    if(!ptr){
        IPAddress::ptr addr = Address::LookupAnyIPAddress(m_host);
        if(!addr){
            LOG_ERROR(g_logger) << "get addr fail: " << m_host;
            return nullptr;
        }
        addr->setPort(m_port);
        Socket::ptr sock = Socket::Create(addr->getFamily(), Socket::TCP);
        if(!sock){
            LOG_ERROR(g_logger) << "create sock fail: " << *addr;
            return nullptr;
        }
        if(!sock->connect(addr)){
            LOG_ERROR(g_logger) << "sock connect fail: " << *addr;
            return nullptr;
        }
        ptr = new HttpConnection(sock);
        ++m_total;
    }
    return HttpConnection::ptr(ptr, std::bind(&HttpConnectionPool::ReleaseConnection
                                                    , std::placeholders::_1, this));
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method
                                , const std::string& url
                                , uint64_t timeout
                                , const std::map<std::string, std::string>& headers
                                , const std::string& body){
    HttpRequest::ptr req = std::make_shared<HttpRequest>();
    req->setPath(url);
    req->setMethod(method);
    req->setClose(false);
    bool has_host = false;
    for(auto & i : headers){
        if(0 == strcasecmp(i.first.c_str(), "connection")){
            if(0 == strcasecmp(i.second.c_str(), "keep-alive"))
                req->setClose(false);
            continue;
        }
        if(0 == strcasecmp(i.first.c_str(), "host"))
            has_host = !i.second.empty();
        req->setHeader(i.first, i.second);
    }
    if(!has_host)
        req->setHeader("Host", m_vhost.empty() ? m_host : m_vhost);
    
    req->setBody(body);
    return doRequest(req, timeout);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method
                                , Uri::ptr uri
                                , uint64_t timeout
                                , const std::map<std::string, std::string>& headers
                                , const std::string& body){
    std::stringstream ss;
    ss  << uri->getPath()
        << (uri->getQuery().empty() ? "" : "?")
        << uri->getQuery()
        << (uri->getFragment().empty() ? "" : "#")
        << uri->getFragment();
    return doRequest(method, ss.str(), timeout, headers, body);                               
}
    
HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req
                                , uint64_t timeout){
    auto conn = getConnection();
    if(!conn)
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_GET_CONNECTION_FAIL
                        , nullptr, "pool host:" + m_host + " port:" + std::to_string(m_port)); 
    auto sock = conn->getSocket();
    if(!sock)
        return std::make_shared<HttpResult>((int)HttpResult::Error::POOL_INVALID_CONNECTION
                        , nullptr, "pool host:" + m_host + " port:" + std::to_string(m_port));

    sock->setRecvTimeout(timeout);
    int rt = conn->sendRequest(req);
    if(0 == rt)
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER
                        , nullptr, "send request close by peer: " + sock->getRemoteAddress()->toString()); 
    if(rt < 0)
        return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR
                        , nullptr, "send request socket error errno: " + std::to_string(errno)
                        + " errstr=" + strerror(errno));
    auto rsp = conn->recvResponse();
    if(!rsp)
        return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT
                        , nullptr, "recv response timeout: " + sock->getRemoteAddress()->toString()
                         + " timeout=" + std::to_string(timeout)); 

    return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "success"); 
}

void HttpConnectionPool::ReleaseConnection(HttpConnection* ptr, HttpConnectionPool* pool){
    ++ptr->m_requestSize;
    if(!ptr->isConnected() 
            || GetCurrentMS() <= ptr->m_createTime + pool->m_maxAliveTime
            || pool->m_maxRequestSize < ptr->m_requestSize){
        delete ptr;
        --pool->m_total;
        return;
    }
    
    Lock lock(pool->m_mutex);
    pool->m_conns.push_back(ptr);
}

__END__
__END__