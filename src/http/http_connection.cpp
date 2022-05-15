#include "http/http_parser.h"
#include "http/http_connection.h"

__OBELISK__
__HTTP__

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


__END__
__END__