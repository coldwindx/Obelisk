#pragma once

#include <memory>
#include <vector>
#include <functional>
#include "noncopyable.h"
#include "iomanager.h"
#include "socket.h"
#include "address.h"

__OBELISK__
__HTTP__

class TcpServer : public std::enable_shared_from_this<TcpServer>, Noncopyable{
public:
    typedef std::shared_ptr<TcpServer> ptr;

    TcpServer(IOManager *worker = IOManager::GetSelf()
                , IOManager *acceptWorker = IOManager::GetSelf());
    virtual ~TcpServer();

    virtual bool bind(Address::ptr addr);
    virtual bool bind(const std::vector<Address::ptr>& addrs
                        , std::vector<Address::ptr>& fails);
    virtual void start();
    virtual void stop();

    uint64_t getRecvTimeout() const { return m_recvTimeout; }
    std::string getName() const { return m_name; }

    void setRecvTimeout(uint64_t v) { m_recvTimeout = v; }
    void setName(const std::string& v) { m_name = v; }

    bool isStop() const { return m_stop; }
protected:
    virtual void handle(Socket::ptr client);
    /**
     * @brief 开始接受连接
     */
    virtual void accept(Socket::ptr sock);
private:
    std::vector<Socket::ptr> m_socks;   // 监听端口
    IOManager *m_worker;                // 工作线程池
    IOManager *m_acceptWorker;          // 负责接受连接的线程池
    uint64_t m_recvTimeout;
    std::string m_name;
    bool m_stop;                        


};

__END__
__END__