#include "log.h"
#include "http/http_server.h"

__OBELISK__
__HTTP__

static Logger::ptr g_logger = LOG_SYSTEM();

HttpServer::HttpServer(bool keepAlive, IOManager *worker, IOManager *acceptWorker)
        : TcpServer(worker, acceptWorker), m_keepAlive(keepAlive){
    m_dispatch.reset(new ServletDispatch());        
}

void HttpServer::handle(Socket::ptr client){
    HttpSession::ptr session(new HttpSession(client));
    do{
        auto req = session->recvRequest();
        if(!req){
            LOG_WARN(g_logger) << "recv http request fail, errno=" << errno 
                << "errstr=" << strerror(errno) << " client=" << *client;
            break;
        }

        HttpResponse::ptr rsp(new HttpResponse(req->getVersion(), req->isClose() || !m_keepAlive));
        m_dispatch->handle(req, rsp, session);
        session->sendResponse(rsp);
    }while(m_keepAlive);
    session->close();
}

__END__
__END__