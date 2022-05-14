#pragma once

#include "http/http.h"
#include "streams/socket_stream.h"

__OBELISK__
__HTTP__
/**
 * @brief 服务端通过accept客户端request请求而产生的socket
 */
class HttpSession : public SocketStream{
public:
    typedef std::shared_ptr<HttpSession> ptr;
    HttpSession(Socket::ptr sock, bool owner = true);

    HttpRequest::ptr recvRequest();
    int sendResponse(HttpResponse::ptr response);
private:
};


__END__
__END__