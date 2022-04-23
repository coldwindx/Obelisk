#pragma once

#include "system.h"
#include "scheduler.h"

__OBELISK__

class IOManager : public Scheduler{
public:
    typedef std::shared_ptr<IOManager> ptr;

    enum Event{ NONE = 0x0, READ = 0x1, WRITE = 0x3 };
private:
    struct FdContext{
        struct EventContext{
            Scheduler* scheduler = nullptr;             // 事件执行的调度器
            Coroutine::ptr coroutine;                   // 事件协程
            std::function<void()> callback;             // 事件回调函数
        };

        EventContext& getContext(Event event);
        void resetContext(EventContext& ctx);
        void triggerEvent(Event event);

        int fd = 0;             // 事件句柄
        EventContext read;      // 读事件
        EventContext write;     // 写事件
        Event event = NONE;     // 注册事件
        Mutex mutex;            
    };
public:
    IOManager(size_t threadsize = 1, const std::string& name = "");
    ~IOManager();
    // 0=success, -1=error
    int addEvent(int fd, Event event, std::function<void()> callback = nullptr);
    bool delEvent(int fd, Event event);
    bool cancelEvent(int fd, Event event);

    bool cancelAll(int fd);

    static IOManager* GetSelf();
protected:
    void tickle() override;
    bool canStop() override;
    void idle() override;
    void resizeContext(size_t size);
private:
    int m_epfd = 0;
    int m_tickleFds[2];
    std::atomic<size_t> m_pendingEventCount = {0};  
    std::vector<FdContext*> m_fdContexts; 
    RWMutex m_rwMutex;
};


__END__