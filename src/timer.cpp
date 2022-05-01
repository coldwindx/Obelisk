#include "timer.h"
#include "log.h"

__OBELISK__

static Logger::ptr g_logger = LOG_SYSTEM();

bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const {
    if(!lhs && !rhs) return false;
    if(!lhs) return true;
    if(!rhs) return false;
    if(lhs->m_next == rhs->m_next)
        return lhs.get() < rhs.get();
    return lhs->m_next < rhs->m_next;
}

Timer::Timer(uint64_t next) : m_next(next) {}

Timer::Timer(uint64_t ms, std::function<void()> callback, bool cycle, TimerManager* manager)
        : m_ms(ms), m_cycle(cycle), m_callback(callback), m_manager(manager) {
    m_next = GetCurrentMS() + m_ms;
}

bool Timer::cancel(){
    WriteLock lock(m_manager->m_rwmutex);
    if(!m_callback) return false;

    m_callback = nullptr;
    auto it = m_manager->m_timers.find(shared_from_this());
    m_manager->m_timers.erase(it);
    return true;
}
bool Timer::refresh(){
    WriteLock lock(m_manager->m_rwmutex);
    if(!m_callback) return false;

    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end())
        return false;
    
    m_manager->m_timers.erase(it);
    m_next = GetCurrentMS() + m_ms;
    m_manager->m_timers.insert(shared_from_this());
    return true;
}
bool Timer::reset(uint64_t ms, bool fromNow){
    if(ms == m_ms && !fromNow) return true;

    WriteLock lock(m_manager->m_rwmutex);
    if(!m_callback) return false;

    auto it = m_manager->m_timers.find(shared_from_this());
    if(it == m_manager->m_timers.end())
        return false;
    
    m_manager->m_timers.erase(it);
    uint64_t start = fromNow ? GetCurrentMS() : (m_next - m_ms);
    m_ms = ms;
    m_next = start + m_ms;
    m_manager->addTimer(shared_from_this(), lock);
    return true;
}

TimerManager::TimerManager(){
    m_previouseTime = GetCurrentMS();
}
TimerManager::~TimerManager(){}

Timer::ptr TimerManager::addTimer(uint64_t ms, std::function<void()> callback, bool cycle){
    Timer::ptr timer(new Timer(ms, callback, cycle, this));
    WriteLock lock(m_rwmutex);
    auto it = m_timers.insert(timer).first;
    bool atFront = (it == m_timers.begin());
    lock.unlock();

    if(atFront) 
        onTimerInsertAtFront();                 
    
    return timer;
}

static void onTimer(std::weak_ptr<void> weakCond, std::function<void()> callback){
    std::shared_ptr<void> tmp = weakCond.lock();
    if(tmp)
        callback();
}

Timer::ptr TimerManager::addContionTimer(uint64_t ms, std::function<void()> callback
                                , std::weak_ptr<void> weakCond, bool cycle){
    return addTimer(ms, std::bind(&onTimer, weakCond, callback), cycle);              
}

uint64_t TimerManager::getNextTimer(){
    ReadLock lock(m_rwmutex);
    m_tickled = false;
    if(m_timers.empty()) return ~0ul;
    
    const Timer::ptr& next = *m_timers.begin();
    uint64_t now = GetCurrentMS();
    if(next->m_next <= now) return 0;
    return next->m_next - now;
}

void TimerManager::listExpiredCallback(std::vector<std::function<void()> >& cbs){
    uint64_t now = GetCurrentMS();
    std::vector<Timer::ptr> expired;
    {
        ReadLock lock(m_rwmutex);
        if(m_timers.empty()) return;
    }
    WriteLock lock2(m_rwmutex);

    bool rollover = detectClockRollover(now);
    if(!rollover && (now < (*m_timers.begin())->m_next))
        return;
    
    Timer::ptr nowTimer(new Timer(now));
    auto it = rollover ? m_timers.end() : m_timers.lower_bound(nowTimer);
    while(it != m_timers.end() && (*it)->m_next == now){
        ++it;
    }
    expired.insert(expired.begin(), m_timers.begin(), it);
    m_timers.erase(m_timers.begin(), it);
    cbs.reserve(expired.size());

    for(auto & timer: expired){
        cbs.push_back(timer->m_callback);
        if(timer->m_cycle){
            timer->m_next = now + timer->m_ms;
            m_timers.insert(timer);
        }else{
            timer->m_callback = nullptr;
        }
    }
}

void TimerManager::addTimer(Timer::ptr timer, WriteLock& lock){
    auto it = m_timers.insert(timer).first;
    bool atFront = (it == m_timers.begin()) && !m_tickled;
    if(atFront)
        m_tickled = true;
    lock.unlock();

    if(atFront)
        onTimerInsertAtFront();
}

bool TimerManager::empty(){
    ReadLock lock(m_rwmutex);
    return m_timers.empty();
}

bool TimerManager::detectClockRollover(uint64_t nowMs){
    bool rollover = false;
    if(nowMs < m_previouseTime && nowMs < (m_previouseTime - 60 * 60* 1000)){
        rollover = true;
    }
    m_previouseTime = nowMs;
    return rollover;
}
__END__