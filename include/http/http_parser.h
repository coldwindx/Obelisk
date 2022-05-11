#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

__OBELISK__
__HTTP__

class HttpRequestParser{
public:
    typedef std::shared_ptr<HttpRequestParser> ptr;

    HttpRequestParser();
    /**
     * @brief 解析http请求
     * @param[in] data: 数据
     * @param[in] len: 数据长度
     * @return   1: 成功 
     *          -1: 失败
     *          >0: 已经处理的字节数v，且data的有效数据为len - v 
     */
    size_t execute(char* data, size_t len);
    int isFinished();
    int hasError();

    HttpRequest::ptr getData() const { return m_data; }
    void setError(int error) { m_error = error; }

    uint64_t getContentLength();
private:
    http_parser m_parser;
    HttpRequest::ptr m_data;
    // 1000: invalid method
    // 1001: invalid http request version
    // 1002: invalid http request field lenght = 0
    int m_error;                
};

class HttpResponseParser{
public:
    typedef std::shared_ptr<HttpResponseParser> ptr;

    HttpResponseParser();

    size_t execute(char* data, size_t len);
    int isFinished();
    int hasError();

    HttpResponse::ptr getData() const { return m_data; }
    void setError(int error) { m_error = error; }

    uint64_t getContentLength();
private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;
    // 1001: invalid http response version
    // 1002: invalid http response field length
    int m_error;
};


__END__
__END__