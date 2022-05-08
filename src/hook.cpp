#include <dlfcn.h>
#include "log.h"
#include "hook.h"
#include "config.h"
#include "coroutine.h"
#include "iomanager.h"
#include "fd_manager.h"

static obelisk::Logger::ptr g_logger = LOG_SYSTEM();

__OBELISK__

static ConfigVar<int>::ptr g_top_connect_timeout 
        = Config::lookup("top.connect.timeout", 5000, "tcp connect, timeout");
static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
    XX(sleep)   \
    XX(usleep)  \
    XX(nanosleep) \
    XX(socket)    \
    XX(connect)    \
    XX(accept)    \
    XX(read)    \
    XX(readv)    \
    XX(recv)    \
    XX(recvfrom)    \
    XX(recvmsg)    \
    XX(write)    \
    XX(writev)    \
    XX(send)    \
    XX(sendto)    \
    XX(sendmsg)    \
    XX(close)    \
    XX(fcntl)    \
    XX(ioctl)    \
    XX(getsockopt)    \
    XX(setsockopt)



void hook_init(){
    static bool is_inited = false;
    if(is_inited) return;

#define XX(name) name ## _f = (name ## _fun) dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX)
#undef XX
}

static uint64_t s_connect_timeout = -1;
struct HookIniter{
    HookIniter(){
        hook_init();
        s_connect_timeout = g_top_connect_timeout->getValue();

        g_top_connect_timeout->addListener([](const int& oldValue, const int& newValue){
            LOG_INFO(g_logger) << "tcp connect timeout changed from " << oldValue
                << " to " << newValue;
            s_connect_timeout = newValue;
        });
    }
};

static HookIniter s_hook_initer;

bool is_hook_enable(){
    return t_hook_enable;
}

void set_hook_enable(bool flag){
    t_hook_enable = flag;
}

__END__

struct timer_info{
    int cancelled = 0;
};

template<typename Origin, typename ... Args>
static ssize_t do_io(int fd, Origin fun, const char * hook_fun_name
                        , uint32_t event, int timeout_so, Args&&... args){
        if(!obelisk::is_hook_enable)
            return fun(fd, std::forward<Args>(args)...);
        obelisk::FdManager::ptr manager = obelisk::FdManager::instance();
        obelisk::FdCtx::ptr ctx = manager->get(fd);
        if(!ctx)
            return fun(fd, std::forward<Args>(args)...);
        if(ctx->isClose()){
            errno = EBADF;
            return -1;
        }
        if(!ctx->isSocket() || ctx->getuserNonblock())
            return fun(fd, std::forward<Args>(args)...);
        uint64_t to = ctx->getTimeout(timeout_so);
        std::shared_ptr<timer_info> tinfo(new timer_info());
retry:
        ssize_t n = fun(fd, std::forward<Args>(args)...);
            // 被系统中断，重试
        while(-1 == n && errno == EINTR)
            n = fun(fd, std::forward<Args>(args)...);
        
        if(-1 == n && EAGAIN == errno){
            obelisk::IOManager* iom = obelisk::IOManager::GetSelf();
            obelisk::Timer::ptr timer;
            std::weak_ptr<timer_info> winfo(tinfo);

            if((uint64_t)-1 != to){
                timer = iom->addContionTimer(to, [winfo, fd, iom, event](){
                    auto t = winfo.lock();
                    if(!t || t->cancelled)
                        return -1;
                    t->cancelled = ETIMEDOUT;
                    iom->cancelEvent(fd, (obelisk::IOManager::Event)event);
                }, winfo);
            }
            int rt = iom->addEvent(fd, (obelisk::IOManager::Event)event);
            if(rt){
                LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                        << fd << ", " << event << ")";
                if(timer)
                    timer->cancel();
                return -1;
            }else{
                obelisk::Coroutine::Yield();
                if(timer)
                    timer->cancel();
                if(tinfo->cancelled){
                    errno = tinfo->cancelled;
                    return -1;
                }
                goto retry;
            }
        }

        return n;
    }

extern "C"{
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX)
#undef XX

    unsigned int sleep(unsigned int seconds){
        if(!obelisk::t_hook_enable)
            return sleep_f(seconds);
        obelisk::Coroutine::ptr c = obelisk::Coroutine::GetSelf();
        obelisk::IOManager* manager = obelisk::IOManager::GetSelf();
        // manager->addTimer(seconds * 1000, std::bind(&obelisk::IOManager::schedule, manager, c, obelisk::thread_id()));
        manager->addTimer(seconds * 1000, [manager, c](){manager->schedule(c, obelisk::thread_id());});
        obelisk::Coroutine::Yield();
        return 0;
    }

    int usleep(useconds_t usec){
        if(!obelisk::t_hook_enable)
            return usleep_f(usec);
        obelisk::Coroutine::ptr c = obelisk::Coroutine::GetSelf();
        obelisk::IOManager* manager = obelisk::IOManager::GetSelf();
        //manager->addTimer(usec / 1000, std::bind(&obelisk::IOManager::schedule, manager, c));
        manager->addTimer(usec / 1000, [manager, c](){manager->schedule(c, obelisk::thread_id());});
        obelisk::Coroutine::Yield();
        return 0;
    }

    int nanosleep(const struct timespec *req, struct timespec *rem){
        if(!obelisk::t_hook_enable)
            return nanosleep_f(req, rem);
        int timeout = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
        obelisk::Coroutine::ptr c = obelisk::Coroutine::GetSelf();
        obelisk::IOManager* manager = obelisk::IOManager::GetSelf();
        manager->addTimer(timeout, [manager, c](){manager->schedule(c, obelisk::thread_id());});
        obelisk::Coroutine::Yield();
        return 0;
    }

    int socket(int domain, int type, int protocol){
        if(!obelisk::t_hook_enable)
            return socket_f(domain, type, protocol);
        int fd = socket_f(domain, type, protocol);
        if(-1 == fd)
            return fd;
        
        obelisk::FdManager::instance()->get(fd, true);
        return fd;
    }

    int connect_with_timeout(int fd, const struct sockaddr *addr, socklen_t addrlen, uint64_t timeout_ms){
        if(!obelisk::t_hook_enable)
            return connect_f(fd, addr, addrlen);

        obelisk::FdCtx::ptr ctx = obelisk::FdManager::instance()->get(fd);
        if(!ctx || ctx->isClose()){
            errno = EBADF;
            return -1;
        }

        if(!ctx->isSocket())
            return connect_f(fd, addr, addrlen);
        if(ctx->getuserNonblock())
            return connect_f(fd, addr, addrlen);

        int n = connect_f(fd, addr, addrlen);
        if(0 == n)
            return 0;
        if(-1 != n || EINPROGRESS != errno)
            return n;

        obelisk::IOManager* iom = obelisk::IOManager::GetSelf();
        obelisk::Timer::ptr timer;
        std::shared_ptr<timer_info> tinfo(new timer_info);
        std::weak_ptr<timer_info> winfo(tinfo);

        if(timeout_ms != (uint64_t)-1){
            timer = iom->addContionTimer(timeout_ms, [winfo, fd, iom](){
                auto t  = winfo.lock();
                if(!t || t->cancelled)
                    return;
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, obelisk::IOManager::WRITE);
            }, winfo);
        }

        int rt = iom->addEvent(fd, obelisk::IOManager::WRITE);
        if(0 == rt){
            obelisk::Coroutine::Yield();
            if(timer)
                timer->cancel();
            if(tinfo->cancelled){
                errno = tinfo->cancelled;
                return -1;
            }
        }else{
            if(timer)
                timer->cancel();
            LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
        }

        int error = 0;
        socklen_t len = sizeof(int);
        if(-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len))
            return -1;
        if(!error)
            return 0;
        errno = error;
        return -1;
    }

    int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen){
        return connect_with_timeout(sockfd, addr, addrlen, obelisk::s_connect_timeout);
    }

    int accept(int sockfd, struct sockaddr *addr, socklen_t * addrlen){
        int fd = do_io(sockfd, accept_f, "accept", obelisk::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
        if(0 <= fd){
            obelisk::FdManager::instance()->get(fd, true);
        }
        return fd;
    }


    // read
    ssize_t read(int fd, void *buf, size_t count){
        return do_io(fd, read_f, "read", obelisk::IOManager::READ, SO_RCVTIMEO, buf, count);
    }

    ssize_t readv(int fd, const struct iovec *iov, int iovcnt){
        return do_io(fd, readv_f, "readv", obelisk::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
    }

    ssize_t recv(int sockfd, void *buf, size_t len, int flags){
        return do_io(sockfd, recv_f, "recv", obelisk::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
    }

    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen){
        return do_io(sockfd, recvfrom_f, "recvfrom", obelisk::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
    }

    ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags){
        return do_io(sockfd, recvmsg_f, "recvmsg", obelisk::IOManager::READ, SO_RCVTIMEO, msg, flags);
    }

    ssize_t write(int fd, const void *buf, size_t count){
        return do_io(fd, write_f, "write", obelisk::IOManager::WRITE, SO_SNDTIMEO, buf, count);
    }

    ssize_t writev(int fd, const struct iovec *iov, int iovcnt){
        return do_io(fd, writev_f, "writev", obelisk::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
    }

    ssize_t send(int sockfd, const void *buf, size_t len, int flags){
        return do_io(sockfd, send_f, "send", obelisk::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags);
    }

    ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,const struct sockaddr *dest_addr, socklen_t addrlen){
        return do_io(sockfd, sendto_f, "sendto", obelisk::IOManager::WRITE, SO_SNDTIMEO, buf, len, flags, dest_addr, addrlen);
    }

    ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags){
        return do_io(sockfd, sendmsg_f, "sendmsg", obelisk::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
    }

    int close(int fd){
        if(!obelisk::t_hook_enable)
            return close_f(fd);

            obelisk::FdCtx::ptr ctx = obelisk::FdManager::instance()->get(fd);
            if(ctx){
                auto iom = obelisk::IOManager::GetSelf();
                if(iom)
                    iom->cancelAll(fd);
                obelisk::FdManager::instance()->del(fd);
            }
            return close_f(fd);
    }

    int fcntl(int fd, int cmd, ...){
        va_list va;
        va_start(va, cmd);
        switch(cmd){
            case F_SETFL:
                {
                    int arg = va_arg(va, int);
                    va_end(va);
                    obelisk::FdCtx::ptr ctx = obelisk::FdManager::instance()->get(fd);
                    if(!ctx || ctx->isClose() || !ctx->isSocket())
                        return fcntl_f(fd, cmd, arg);
                    ctx->setUserNonblock(arg & O_NONBLOCK);
                    if(ctx->getSysNonblock())
                        arg |= O_NONBLOCK;
                    else
                        arg & ~O_NONBLOCK;
                    
                    return fcntl_f(fd, cmd, arg);
                }
                break;
            case F_GETFL:
                {
                    va_end(va);
                    int arg = fcntl_f(fd, cmd);
                    obelisk::FdCtx::ptr ctx = obelisk::FdManager::instance()->get(fd);
                    if(!ctx || ctx->isClose() || ctx->isSocket())
                        return arg;
                    if(ctx->getuserNonblock())
                        return arg | O_NONBLOCK;
                    else    
                        return arg & ~O_NONBLOCK;
                }
                break;
            case F_DUPFD:
            case F_DUPFD_CLOEXEC:
            case F_SETFD:
            case F_SETOWN:
            case F_SETSIG:
            case F_SETLEASE:
            case F_NOTIFY:
            case F_SETPIPE_SZ:
                {
                    int arg = va_arg(va, int);
                    va_end(va);
                    return fcntl_f(fd, cmd, arg);
                }
                break;
            case F_GETFD:
            case F_GETOWN:
            case F_GETSIG:
            case F_GETLEASE:
            case F_GETPIPE_SZ:
                {
                    va_end(va);
                    return fcntl_f(fd, cmd);
                }
                break;
            case F_SETLK:
            case F_SETLKW:
            case F_GETLK:
                {
                    struct flock* arg = va_arg(va, struct flock*);
                    va_end(va);
                    return fcntl_f(fd, cmd, arg);
                }
                break;
            case F_GETOWN_EX:
            case F_SETOWN_EX:
                {
                    struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
                    va_end(va);
                    return fcntl_f(fd, cmd, arg);
                }
                break;
            default:
                va_end(va);
                return fcntl_f(fd, cmd);
                break;
        }
    }

    int ioctl(int fd, unsigned long request, ...){
        va_list va;
        va_start(va, request);
        void* arg = va_arg(va, void*);
        va_end(va);

        if(FIONBIO == request){
            bool user_nonblock = !!*(int*)arg;
            obelisk::FdCtx::ptr ctx = obelisk::FdManager::instance()->get(fd);
            if(!ctx || ctx->isClose() || ctx->isSocket())
                return ioctl_f(fd, request, arg);
            ctx->setUserNonblock(user_nonblock);
        }
        return ioctl_f(fd, request, arg);
    }

    int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen){
        return getsockopt_f(sockfd, level, optname, optval, optlen);
    }
    
    int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen){
        if(!obelisk::t_hook_enable)
            return setsockopt_f(sockfd, level, optname, optval, optlen);
        if(SOL_SOCKET == level){
            if(SO_RCVTIMEO == optname || SO_SNDTIMEO == optname){
                obelisk::FdCtx::ptr ctx = obelisk::FdManager::instance()->get(sockfd);
                if(ctx){
                    const timeval* v = (const timeval*)optval;
                    ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
                }
            }
        }
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
}
