#pragma once

#include <atomic>
#include <list>
#include "mutex.h"
#include "http/uri.h"
#include "http/http.h"
#include "streams/socket_stream.h"

__OBELISK__
__HTTP__

class HttpConnectionPool;

struct HttpResult{
    typedef std::shared_ptr<HttpResult> ptr;
    enum class Error{
        OK = 0,
        INvALID_URL = 1,
        INvALID_HOST = 2,
        CONNECT_FAIL = 3,
        SEND_CLOSE_BY_PEER = 4,
        SEND_SOCKET_ERROR = 5,
        TIMEOUT = 6,
        CREATE_SOCKET_ERROR = 7,
        POOL_GET_CONNECTION_FAIL = 8,
        POOL_INVALID_CONNECTION = 9
    };

    HttpResult(int res, HttpResponse::ptr rsp, const std::string& err)
            : result(res), error(err), response(rsp) {}

    std::string toString() const;

    int result;
    std::string error;
    HttpResponse::ptr response;
};

/**
 * @brief 客户端通过accept服务器端response回复而产生的socket
 */
class HttpConnection : public SocketStream{
    friend class HttpConnectionPool;
public:
    typedef std::shared_ptr<HttpConnection> ptr;
    HttpConnection(Socket::ptr sock, bool owner = true);

    HttpResponse::ptr recvResponse();
    int sendRequest(HttpRequest::ptr req);

    static HttpResult::ptr DoRequest(HttpMethod method
                                        , const std::string& url
                                        , uint64_t timeout
                                        , const std::map<std::string, std::string>& headers = {}
                                        , const std::string& body = "");
    static HttpResult::ptr DoRequest(HttpMethod method
                                        , Uri::ptr uri
                                        , uint64_t timeout
                                        , const std::map<std::string, std::string>& headers = {}
                                        , const std::string& body = "");
    static HttpResult::ptr DoRequest(HttpRequest::ptr req
                                        , Uri::ptr uri
                                        , uint64_t timeout);
private:
    uint64_t m_createTime;
    uint64_t m_requestSize = 0;
};

class HttpConnectionPool{
public:
    typedef std::shared_ptr<HttpConnectionPool> ptr;

    HttpConnectionPool(const std::string& host
                        , const std::string& vhost
                        , uint32_t port
                        , uint32_t maxSize
                        , uint32_t maxAliveTime
                        , uint32_t maxRequestSize);

    HttpConnection::ptr getConnection();

    HttpResult::ptr doRequest(HttpMethod method
                                , const std::string& url
                                , uint64_t timeout
                                , const std::map<std::string, std::string>& headers = {}
                                , const std::string& body = "");

    HttpResult::ptr doRequest(HttpMethod method
                                , Uri::ptr uri
                                , uint64_t timeout
                                , const std::map<std::string, std::string>& headers = {}
                                , const std::string& body = "");
    
    HttpResult::ptr doRequest(HttpRequest::ptr req, uint64_t timeout);
private:
    static void ReleaseConnection(HttpConnection* ptr, HttpConnectionPool* pool);
private:
    std::string m_host;
    std::string m_vhost;
    uint32_t    m_port;
    uint32_t    m_maxSize;
    uint32_t    m_maxAliveTime;
    uint32_t    m_maxRequestSize;

    Mutex m_mutex;
    std::list<HttpConnection*> m_conns;
    std::atomic<int32_t> m_total = {0};
};
__END__
__END__