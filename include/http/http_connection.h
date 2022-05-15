#pragma once

#include "http/http.h"
#include "streams/socket_stream.h"

__OBELISK__
__HTTP__
/**
 * @brief 客户端通过accept服务器端response回复而产生的socket
 */
class HttpConnection : public SocketStream{
public:
    typedef std::shared_ptr<HttpConnection> ptr;
    HttpConnection(Socket::ptr sock, bool owner = true);

    HttpResponse::ptr recvResponse();
    int sendRequest(HttpRequest::ptr req);
private:
};


__END__
__END__