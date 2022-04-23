#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include "iomanager.h"

__OBELISK__

static Logger::ptr g_logger = LOG_SYSTEM();


IOManager::FdContext::EventContext& IOManager::FdContext::getContext(Event event){
    switch(event){
        case IOManager::READ: return read;
        case IOManager::WRITE: return write;
        default: OBELISK_ASSERT2(false, "getContext");
    }
}
void IOManager::FdContext::resetContext(IOManager::FdContext::EventContext& ctx){
    ctx.scheduler = nullptr;
    ctx.coroutine.reset();
    ctx.callback = nullptr;
}
void IOManager::FdContext::triggerEvent(IOManager::Event event){
    OBELISK_ASSERT(this->event & event);
    this->event = (Event)(this->event & ~event);
    EventContext& ctx = getContext(event);
    if(ctx.callback){
        ctx.scheduler->schedule(&ctx.callback);
    }else{
        ctx.scheduler->schedule(&ctx.coroutine);
    }
    ctx.scheduler = nullptr;
}


IOManager::IOManager(size_t threadsize, const std::string& name)
        : Scheduler(threadsize, name) {
    m_epfd = epoll_create(5000);
    OBELISK_ASSERT(0 < m_epfd);
    
    int rt = pipe(m_tickleFds);
    OBELISK_ASSERT(rt);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    OBELISK_ASSERT(rt);

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    OBELISK_ASSERT(rt);

    resizeContext(32);
    this->start();
}

IOManager::~IOManager(){
    this->stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for(size_t i = 0, m_fdContexts.size(); i < n; ++i){
        if(m_fdContexts[i]){
            delete m_fdContexts[i];
        }
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> callback){
    FdContext* fdCtx = nullptr;
    ReadLock lock(m_rwMutex);
    if(fd < m_fdContexts.size()){
        fdCtx = m_fdContexts[fd];
        lock.unlock();
    }else{
        lock.unlock();
        WriteLock lock(m_rwMutex);
        resizeContext(1.5 * m_fdContexts.size());
        fdCtx = m_fdContexts[fd];
    }

    Lock lock2(fd_ctx->mutex);
    if(fdCtx->event & event){
        LOG_ERROR(g_logger) << "addEvent assert fd=" << fd 
                " ,event=" << event << " ,fd_ctx.event=" << fdCtx->event;
        OBELISK_ASSERT(!(fdCtx->event & event));
    }

    int op = fdCtx->event ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | fdCtx->event | event;
    epevent.data.ptr = fdCtx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op
            << ", " << fd << ", " << epevent.events << "): " << rt
            << " {" << errno << "} {" << strerror(errno) << "}";
        return -1;
    }

    ++m_pendingEventCount;
    fdCtx->event = (Event)(fdCtx->event | event);
    FdContext::EventContext& eventCtx = fdCtx->getContext(event);
    OBELISK_ASSERT(!(eventCtx.scheduler || eventCtx.coroutine || eventCtx.callback));

    eventCtx.scheduler = Scheduler::GetSelf();
    if(callback){
        eventCtx.callback.swap(cb);
    }else{
        eventCtx.coroutine = Coroutine::GetSelf();
        OBELISK_ASSERT(eventCtx.coroutine->getState() == Coroutine::EXEC);
    }

    return 0;
}

bool IOManager::delEvent(int fd, Event event){
    ReadLock lock(m_rwMutex);
    if(m_fdContexts.size() <= fd)
        return false;
    FdContext * fdCtx = m_fdContexts[fd];
    lock.unlock();

    Lock lock2(fdCtx->mutex);
    if(!(fdCtx->event & event))
        return false;

    Event newEvents = (Event)(fdCtx->event & ~event);
    int op = newEvents ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | newEvents;
    epevent.data.ptr = fdCtx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op
            << ", " << fd << ", " << epevent.events << "): " << rt
            << " {" << errno << "} {" << strerror(errno) << "}";
        return false;
    }

    --m_pendingEventCount;
    fdCtx->event = newEvents;
    FdContext::EventContext& eventCtx = fdCtx->getContext(event);
    fdCtx->resetContext(eventCtx);

    return true;
}

bool IOManager::cancelEvent(int fd, Event event){
    ReadLock lock(m_rwMutex);
    if(m_fdContexts.size() <= fd)
        return false;
    FdContext * fdCtx = m_fdContexts[fd];
    lock.unlock();

    Lock lock2(fdCtx->mutex);
    if(!(fdCtx->event & event))
        return false;

    Event newEvents = (Event)(fdCtx->event & ~event);
    int op = newEvents ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | newEvents;
    epevent.data.ptr = fdCtx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op
            << ", " << fd << ", " << epevent.events << "): " << rt
            << " {" << errno << "} {" << strerror(errno) << "}";
        return false;
    }

    eventCtx->triggerEvent(event);
    --m_pendingEventCount;
    return true;
}

bool IOManager::cancelAll(int fd){
    ReadLock lock(m_rwMutex);
    if(m_fdContexts.size() <= fd)
        return false;
    FdContext * fdCtx = m_fdContexts[fd];
    lock.unlock();

    Lock lock2(fdCtx->mutex);
    if(!fdCtx->event) return false;

    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fdCtx;

    int rt = epoll_ctl(m_epfd, EPOLL_CTL_DEL, fd, &epevent);
    if(rt){
        LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << EPOLL_CTL_DEL
            << ", " << fd << ", " << epevent.events << "): " << rt
            << " {" << errno << "} {" << strerror(errno) << "}";
        return false;
    }

    if(fdCtx->event & READ){
        eventCtx->triggerEvent(READ);
        --m_pendingEventCount;
    }

    if(fdCtx->event & WRITE){
        eventCtx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    OBELISK_ASSERT(fdCtx->event == 0);
    return true;
}

IOManager* IOManager::GetSelf(){
    return dynamic_cast<IOManager*>(Scheduler::GetSelf());
}

void IOManager::tickle();
bool IOManager::canStop();
void IOManager::idle();
void IOManager::resizeContext(size_t size){
    m_fdContexts.resize(size);
    for(size_t i = 0, n = m_fdContexts.size(); i < n; ++i){
        if(!m_fdContexts[i]){
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}
__END__