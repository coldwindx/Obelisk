#pragma once

#include "system.hpp"
#include "http/http_server.h"

__OBELISK__


class Application {
public:
    Application();

    static Application* instance() { return s_instance;}
    bool init(int argc, char** argv);
    bool run();
private:
    int main(int argc, char** argv);
    int runCoroutine();
private:
    int m_argc = 0;
    char** m_argv = nullptr;

    std::vector<http::HttpServer::ptr> m_servers;
    static Application* s_instance;
};


__END__