#pragma once

#include "http/tcp_server.h"
#include "http/http_session.h"
#include "http/servlet.h"

__OBELISK__
__HTTP__

class HttpServer : public TcpServer{
public:
    typedef std::shared_ptr<HttpServer> ptr;

    HttpServer(bool keepAlive = false
                , IOManager *worker = IOManager::GetSelf()
                , IOManager *acceptWorker = IOManager::GetSelf());
    
    ServletDispatch::ptr getServletDispatch() const { return m_dispatch; }
    void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v; }
protected:
    virtual void handle(Socket::ptr client) override;
private:
    bool m_keepAlive;               // 是否长连接
    ServletDispatch::ptr m_dispatch;
};

__END__
__END__