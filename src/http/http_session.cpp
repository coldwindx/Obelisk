#include "http/http_parser.h"
#include "http/http_session.h"

__OBELISK__
__HTTP__

HttpSession::HttpSession(Socket::ptr sock, bool owner)
        : SocketStream(sock, owner) {

}
HttpRequest::ptr HttpSession::recvRequest(){
    HttpRequestParser::ptr parser(new HttpRequestParser);
    uint64_t buff_size = HttpRequestParser::GetHttpRequestBufferSize();
    std::shared_ptr<char> buffer(new char[buff_size], std::default_delete<char[]>());

    char *data = buffer.get();
    int offset = 0;
    do{
        int len = read(data + offset, buff_size - offset);
        if(len <= 0) {
            close();
            return nullptr;
        }
        len += offset;
        size_t nparse = parser->execute(data, len + offset);
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
            if(readFixSize(&body[body.size()], length) <= 0){
                close();
                return nullptr;
            }
        }
        parser->getData()->setBody(body);
    }
    return parser->getData();
}

int HttpSession::sendResponse(HttpResponse::ptr response){
    std::stringstream ss;
    ss << *response;
    std::string data = ss.str();
    return writeFixSize(data.c_str(), data.size());
}


__END__
__END__