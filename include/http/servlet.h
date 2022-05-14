#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "mutex.h"
#include "http/http.h"
#include "http/http_session.h"

__OBELISK__
__HTTP__

class Servlet{
public:
    typedef std::shared_ptr<Servlet> ptr;
    Servlet(const std::string& name) : m_name(name) {}
    virtual ~Servlet() {}

    virtual int32_t handle(HttpRequest::ptr request
                            , HttpResponse::ptr response
                            , HttpSession::ptr session) = 0;

    virtual const std::string& getName() const { return m_name; }
protected:
    std::string m_name;
};

class FunctionServlet : public Servlet{
public:
    typedef std::shared_ptr<FunctionServlet> ptr;
    typedef std::function<int32_t(HttpRequest::ptr, HttpResponse::ptr, HttpSession::ptr)> callback;
    FunctionServlet(callback cb);

    virtual int32_t handle(HttpRequest::ptr request
                            , HttpResponse::ptr response
                            , HttpSession::ptr session) override;
private:
    callback m_cb;
};

class ServletDispatch : public Servlet{
public:
    typedef std::shared_ptr<ServletDispatch> ptr;
    ServletDispatch();

    virtual int32_t handle(HttpRequest::ptr request
                            , HttpResponse::ptr response
                            , HttpSession::ptr session) override;


    void addServlet(const std::string& uri, Servlet::ptr slt);
    void addServlet(const std::string& uri, FunctionServlet::callback cb);
    void addGlobServlet(const std::string& urt, Servlet::ptr slt);
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

    void delServlet(const std::string& uri);
    void delGlobServlet(const std::string& uri);

    Servlet::ptr getDefault() const { return m_default; }
    void setDefault(Servlet::ptr v) { m_default = v; }

    Servlet::ptr getServlet(const std::string& uri);
    Servlet::ptr getGlobServlet(const std::string& uri);
    Servlet::ptr getMatchServlet(const std::string& uri);
private:
    RWMutex m_mutex;
    // uri ---> servlet(精准匹配)
    std::unordered_map<std::string, Servlet::ptr> m_datas;
    // uri ---> servlet(模糊匹配)
    std::vector<std::pair<std::string, Servlet::ptr> > m_globs;
    // 默认servlet
    Servlet::ptr m_default;
};

class NotFoundServlet : public Servlet{
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;
    NotFoundServlet();
    virtual int32_t handle(HttpRequest::ptr request
                            , HttpResponse::ptr response
                            , HttpSession::ptr session) override;

};
__END__
__END__
