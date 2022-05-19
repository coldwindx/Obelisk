#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <algorithm>
#include "log.h"
#include "assert.hpp"
#include "iomanager.h"
#include "macro.h"

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
    OBELISK_ASSERT(this->events & event);
    this->events = (Event)(this->events & ~event);
    EventContext& ctx = getContext(event);

    Scheduler* scheduler = Scheduler::GetSelf();
    if(ctx.callback)
        scheduler->schedule(ctx.callback);
    if(ctx.coroutine)
        scheduler->schedule(ctx.coroutine);
}


IOManager::IOManager(size_t threadsize, const std::string& name)
        : Scheduler(threadsize, name) {
    m_epfd = epoll_create(5000);
    OBELISK_ASSERT(0 < m_epfd);
    
    int rt = pipe(m_tickleFds);
    OBELISK_ASSERT(!rt);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = m_tickleFds[0];

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    OBELISK_ASSERT(!rt);

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);
    OBELISK_ASSERT(!rt);

    resizeContext(32);
    this->start();
}

IOManager::~IOManager(){
    this->stop();
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for(size_t i = 0, n = m_fdContexts.size(); i < n; ++i){
        if(m_fdContexts[i]){
            delete m_fdContexts[i];
        }
    }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> callback){
    FdContext* fdCtx = nullptr;
    ReadLock lock(m_rwMutex);
    if(fd < (int)m_fdContexts.size()){
        fdCtx = m_fdContexts[fd];
        lock.unlock();
    }else{
        lock.unlock();
        WriteLock lock(m_rwMutex);
        resizeContext(1.5 * fd);
        fdCtx = m_fdContexts[fd];
    }

    Lock lock2(fdCtx->mutex);
    if(fdCtx->events & event){
        LOG_ERROR(g_logger) << "addEvent assert fd=" << fd  
                << " ,event=" << event << " ,fd_ctx.event=" << fdCtx->events;
        OBELISK_ASSERT(!(fdCtx->events & event));
    }

    int op = fdCtx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | fdCtx->events | event;
    epevent.data.ptr = fdCtx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt){
        LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op
            << ", " << fd << ", " << epevent.events << "): " << rt
            << " {" << errno << "} {" << strerror(errno) << "}";
        return -1;
    }

    ++m_pendingEventCount;
    fdCtx->events = (Event)(fdCtx->events | event);
    FdContext::EventContext& eventCtx = fdCtx->getContext(event);
   //TODO[BUG] OBELISK_ASSERT(!(eventCtx.scheduler || eventCtx.coroutine || eventCtx.callback));

    eventCtx.scheduler = Scheduler::GetSelf();
    if(callback){
        eventCtx.callback.swap(callback);
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
    if(!(fdCtx->events & event))
        return false;

    Event newEvents = (Event)(fdCtx->events & ~event);
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
    fdCtx->events = newEvents;
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
    if(!(fdCtx->events & event))
        return false;

    Event newEvents = (Event)(fdCtx->events & ~event);
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

    fdCtx->triggerEvent(event);
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
    if(!fdCtx->events) return false;

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

    if(fdCtx->events & READ){
        fdCtx->triggerEvent(READ);
        --m_pendingEventCount;
    }

    if(fdCtx->events & WRITE){
        fdCtx->triggerEvent(WRITE);
        --m_pendingEventCount;
    }

    OBELISK_ASSERT(fdCtx->events == 0);
    return true;
}

IOManager* IOManager::GetSelf(){
    return dynamic_cast<IOManager*>(Scheduler::GetSelf());
}

void IOManager::tickle(){
    if(0 == getActiveThreadCount())
        return;
    int rt = write(m_tickleFds[1], "T", 1);
    OBELISK_ASSERT(rt == 1);
    
}
bool IOManager::canStop(){
    return Scheduler::canStop() 
        && (m_pendingEventCount == 0)
        && empty();
}

bool IOManager::canStop(uint64_t& timeout){
    timeout = getNextTimer();
    return Scheduler::canStop() 
        && (m_pendingEventCount == 0)
        && (timeout == ~0ul);
}
void IOManager::idle(){
    epoll_event* events = new epoll_event[64];
    std::shared_ptr<epoll_event> sharedEvents(events, std::default_delete<epoll_event[]>());
    while(true){
        uint64_t nextTimeout = 0;
        if(canStop(nextTimeout)){
            // LOG_DEBUG(g_logger) << "name=" << getName() << " idle stopping exit";
            break;
        }
    
        int rt = 0;
        do{
            static const int MAX_TIMEOUT = 3000;
            if(~0ul == nextTimeout)
                nextTimeout = MAX_TIMEOUT;
            if(MAX_TIMEOUT < (int)nextTimeout)
                nextTimeout = MAX_TIMEOUT;
            rt = epoll_wait(m_epfd, events, 64, nextTimeout);
        }while(rt < 0 && errno == EINTR);   
            
        std::vector<std::function<void()> > cbs;
        listExpiredCallback(cbs);
        if(!cbs.empty()){
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

        for(size_t i = 0; i < rt; ++i){
            epoll_event& event = events[i];
            if(event.data.fd == m_tickleFds[0]){
                uint8_t dummy;
                while(0 < read(m_tickleFds[0], &dummy, 1)){}
                continue;
            }
            FdContext* fdCtx = (FdContext*)event.data.ptr;
            Lock lock(fdCtx->mutex);
            if(event.events & (EPOLLERR | EPOLLHUP)){
                event.events |= (EPOLLOUT | EPOLLIN) & fdCtx->events;
            }
            int realEvent = NONE;
            if(event.events & EPOLLIN){
                realEvent |= READ;
            }
            if(event.events & EPOLLOUT){
                realEvent |= WRITE;
            }

            if((fdCtx->events & realEvent) == NONE){
                continue;
            }
            int leftEvents = (fdCtx->events & ~realEvent);
            int op = leftEvents ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;

            event.events = EPOLLET | leftEvents;

            int rt2 = epoll_ctl(m_epfd, op, fdCtx->fd, &event);
            if(rt2){
                LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", " << op
                    << ", " << fdCtx->fd << ", " << event.events << "): " << rt2
                    << " {" << errno << "} {" << strerror(errno) << "}";
                continue;
            }

            if(realEvent & READ){
                fdCtx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(realEvent & WRITE){
                fdCtx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }
        Coroutine::ptr c = Coroutine::GetSelf();
        auto raw = c.get();
        c.reset();

        raw->swapOut();
    }
}
void IOManager::resizeContext(size_t size){
    m_fdContexts.resize(size);
    for(size_t i = 0, n = m_fdContexts.size(); i < n; ++i){
        if(!m_fdContexts[i]){
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

void IOManager::onTimerInsertAtFront() {
    tickle();
}
__END__