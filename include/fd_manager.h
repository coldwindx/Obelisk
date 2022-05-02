#include <memory>
#include <vector>
#include "system.h"
#include "thread/thread.h"
#include "coroutine/iomanager.h"

__OBELISK__

class FdCtx : public std::enable_shared_from_this<FdCtx>{
public:
    typedef std::shared_ptr<FdCtx> ptr;

    FdCtx(int fd);
    ~FdCtx();

    bool init();
    bool isInited() const { return m_isInited; }
    bool isSocket() const { return m_isSocket; }
    bool isClose() const { return m_isClosed; }
    bool close();

    void setUserNonblock(bool v) { m_userNonblock = v; }
    bool getuserNonblock() const { return m_userNonblock; }

    void setSysNonblock(bool v) { m_sysNonblock = v; }
    bool getSysNonblock() const { return m_sysNonblock; }

    void setTimeout(int type, uint64_t v);
    uint64_t getTimeout(int type);


private:
    bool m_isInited = true;
    bool m_isSocket = true;
    bool m_sysNonblock = true;
    bool m_userNonblock = true;
    bool m_isClosed = true;
    int m_fd;

    uint64_t m_recvTimeout;
    uint64_t m_sendTimeout;

};

class FdManager{
public:
    typedef std::shared_ptr<FdManager> ptr;

    static FdManager::ptr instance();

    FdCtx::ptr get(int fd, bool auto_create = false);
    void del(int fd);
private:
    FdManager();
private:
    RWMutex m_mutex;
    std::vector<FdCtx::ptr> m_datas;
    
};
__END__