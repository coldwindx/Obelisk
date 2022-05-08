#include <sys/stat.h>
#include "fd_manager.h"
#include "hook.h"

__OBELISK__
FdCtx::FdCtx(int fd) 
        : m_isInited(false)
        , m_isSocket(false)
        , m_sysNonblock(false)
        , m_userNonblock(false)
        , m_isClosed(false)
        , m_fd(fd)
        , m_recvTimeout(-1)
        , m_sendTimeout(-1){
    this->init();
}

FdCtx::~FdCtx(){

}

bool FdCtx::init(){
    if(m_isInited) return true;
    m_recvTimeout = -1;
    m_sendTimeout = -1;

    struct stat fd_stat;
    if(-1 == fstat(m_fd, &fd_stat)){
        m_isInited = false;
        m_isSocket = false;
    }else{
        m_isInited = true;
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
    }

    if(m_isSocket){
        int flags = fcntl_f(m_fd, F_GETFL, 0);
        if(!(flags & O_NONBLOCK)){
            fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
        }
        m_sysNonblock = true;
    }else{
        m_sysNonblock = false;
    }
    m_userNonblock = false;
    m_isClosed = false;
    return m_isInited;
}

void FdCtx::setTimeout(int type, uint64_t v){
    if(SO_RCVTIMEO == type)
        m_recvTimeout = v;
    if(SO_SNDTIMEO == type)
        m_sendTimeout = v;
}
uint64_t FdCtx::getTimeout(int type){
    if(SO_RCVTIMEO == type)
        return m_recvTimeout;
    if(SO_SNDTIMEO == type)
        return m_sendTimeout;
}

FdManager::FdManager(){
    m_datas.resize(64);
}

FdManager::ptr FdManager::instance(){
    static FdManager::ptr manager(new FdManager());
    return manager;
}

FdCtx::ptr FdManager::get(int fd, bool auto_create){
    ReadLock lock(m_mutex);
    if(m_datas.size() <= fd){
        if(false == auto_create)
            return nullptr;
        m_datas.resize(fd * 1.5);
    }else{
        if(m_datas[fd] || !auto_create)
            return m_datas[fd];
    }

    lock.unlock();

    WriteLock lock2(m_mutex);
    FdCtx::ptr ctx(new FdCtx(fd));
    m_datas[fd] = ctx;
    return ctx;
}
void FdManager::del(int fd){
    WriteLock wlock(m_mutex);
    if(m_datas.size() <= fd)
        return;
    
    m_datas[fd].reset();
}
__END__