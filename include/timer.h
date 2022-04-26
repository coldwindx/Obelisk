#pragma once

#include <vector>
#include <set>
#include "system.h"
#include "thread/mutex.h"
#include "thread/thread.h"

__OBELISK__
class TimerManager;

class Timer : public std::enable_shared_from_this<Timer>{
    friend class TimerManager;
public:
    typedef std::shared_ptr<Timer> ptr;

    bool cancel();
    bool refresh();
    bool reset(uint64_t ms, bool fromNow);
private:
    Timer(uint64_t next);
    Timer(uint64_t ms, std::function<void()> callback, bool cycle, TimerManager* manager);


private:
    bool m_cycle = false;               // 是否循环定时器
    uint64_t m_ms = 0;                  // 执行周期
    uint64_t m_next = 0;                // 精确的执行时间
    std::function<void()> m_callback;
    TimerManager* m_manager = nullptr;
private:
    struct Comparator{
        bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
    };
};

class TimerManager{
    friend class Timer;
public:
    TimerManager();
    virtual ~TimerManager();

    Timer::ptr addTimer(uint64_t ms, std::function<void()> callback, bool cycle = false);
    Timer::ptr addContionTimer(uint64_t ms, std::function<void()> callback
                                , std::weak_ptr<void> weakCond, bool cycle = false);
    uint64_t getNextTimer();
    void listExpiredCallback(std::vector<std::function<void()> >& cbs);
protected:
    virtual void onTimerInsertAtFront() = 0;            // 与当前定时器对比，优先唤醒
    void addTimer(Timer::ptr p, WriteLock& lock);
    bool empty();
private:
    bool detectClockRollover(uint64_t nowMs);           // 检测系统时间的修改
private:
    RWMutex m_rwmutex;
    std::set<Timer::ptr> m_timers;
    bool m_tickled = false;
    uint64_t m_previouseTime = 0;
};
__END__