#include <fnmatch.h>
#include "http/servlet.h"

__OBELISK__
__HTTP__

FunctionServlet::FunctionServlet(callback cb)
        : Servlet("FunctionServlet"), m_cb(cb){

}

int32_t FunctionServlet::handle(HttpRequest::ptr request
                                , HttpResponse::ptr response
                                , HttpSession::ptr session) {
    return m_cb(request, response, session);                                
}

ServletDispatch::ServletDispatch() : Servlet("ServletDispatch"){
    m_default.reset(new NotFoundServlet());
}

int32_t ServletDispatch::handle(HttpRequest::ptr request
                                , HttpResponse::ptr response
                                , HttpSession::ptr session){
    auto slt = getMatchServlet(request->getPath());
    if(slt){
        slt->handle(request, response, session);
    }
    return 0;
}


void ServletDispatch::addServlet(const std::string& uri, Servlet::ptr slt){
    WriteLock lock(m_mutex);
    m_datas[uri] = slt;
}
void ServletDispatch::addServlet(const std::string& uri, FunctionServlet::callback cb){
    WriteLock lock(m_mutex);
    m_datas[uri].reset(new FunctionServlet(cb));
}
void ServletDispatch::addGlobServlet(const std::string& uri, Servlet::ptr slt){
    WriteLock lock(m_mutex);
    for(auto it = m_globs.begin(); it != m_globs.end(); ++it){
        if(it->first == uri){
            m_globs.erase(it);
            break;
        }
    }
    m_globs.push_back(std::make_pair(uri, slt));
}
void ServletDispatch::addGlobServlet(const std::string& uri, FunctionServlet::callback cb){
    return addGlobServlet(uri, FunctionServlet::ptr(new FunctionServlet(cb)));
}

void ServletDispatch::delServlet(const std::string& uri){
    WriteLock lock(m_mutex);
    m_datas.erase(uri);
}
void ServletDispatch::delGlobServlet(const std::string& uri){
    WriteLock lock(m_mutex);
    for(auto it = m_globs.begin(); it != m_globs.end(); ++it){
        if(it->first == uri){
            m_globs.erase(it);
            break;
        }
    }
}

Servlet::ptr ServletDispatch::getServlet(const std::string& uri){
    ReadLock lock(m_mutex);
    auto it = m_datas.find(uri);
    return it == m_datas.end() ? nullptr : it->second;
}
Servlet::ptr ServletDispatch::getGlobServlet(const std::string& uri){
    ReadLock lock(m_mutex);
    for(auto it = m_globs.begin(); it != m_globs.end(); ++it)
        if(!fnmatch(it->first.c_str(), uri.c_str(), 0))
            return it->second;
    return nullptr;  
}
Servlet::ptr ServletDispatch::getMatchServlet(const std::string& uri){
    ReadLock lock(m_mutex);
    auto it = m_datas.find(uri);
    if(it != m_datas.end())
        return it->second;
    for(auto jt = m_globs.begin(); jt != m_globs.end(); ++jt)
        if(!fnmatch(jt->first.c_str(), uri.c_str(), 0))
            return jt->second;
    return m_default;
}


NotFoundServlet::NotFoundServlet() : Servlet("NotFoundServlet"){

}
int32_t NotFoundServlet::handle(HttpRequest::ptr request
                                , HttpResponse::ptr response
                                , HttpSession::ptr session){
    static const std::string& RSP_BODY = "<html><head><title>404 Not Found"
        "</title></head><body><center><h1>404 Not Found</h1></center>"
        "<hr><center>obelisk/1.0.0</center></body></html>";
    response->setStatus(HttpStatus::NOT_FOUND);
    response->setHeader("Server", "obelisk/1.0.0");
    response->setHeader("Content-Type", "text/html");
    response->setBody(RSP_BODY);
    return 0;
}


__END__
__END__
