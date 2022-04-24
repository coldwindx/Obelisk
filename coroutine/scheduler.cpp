#include "coroutine_macro.h"
#include "scheduler.h"

__OBELISK__

static Logger::ptr g_logger = LOG_SYSTEM();

static thread_local Scheduler* t_scheduler = nullptr;

Scheduler::Scheduler(size_t threadsize, const std::string& name) 
        : m_name(name){
    OBELISK_ASSERT(0 < threadsize);
    m_threadCount = threadsize;
}

Scheduler::~Scheduler(){
    OBELISK_ASSERT(m_stopping);
    if(this == GetSelf())
        t_scheduler = nullptr;
}

Scheduler* Scheduler::GetSelf(){
    return t_scheduler;
}

void Scheduler::start(){
    Lock lock(m_mutex);
    if(m_stopping) return;
    OBELISK_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount; ++i){
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
            , m_name + "-" + std::to_string(i)));
    }
}

void Scheduler::stop(){
    m_stopping = true;
    for(size_t i = 0; i < m_threadCount; ++i)
        m_threads[i]->join();
}

void Scheduler::schedule(std::function<void()> func, int threadId){
    Lock lock(m_mutex);
    Coroutine::ptr p(new Coroutine(func));
    if(m_coroutines.empty()) tickle();
    m_coroutines.push_back(std::make_pair(threadId, p));
}

void Scheduler::schedule(Coroutine::ptr c, int threadId){
    Lock lock(m_mutex);
    if(m_coroutines.empty()) tickle();
    m_coroutines.push_back(std::make_pair(threadId, c));
}


void Scheduler::run(){
    LOG_DEBUG(g_logger) << "Scheduler::run()";
    t_scheduler = this;

    Coroutine::GetSelf();
    Coroutine::ptr idleC(new Coroutine(std::bind(&Scheduler::idle, this)));
    Coroutine::ptr c;
    while(true){
        c.reset();
        {
            Lock lock(m_mutex);
            // 取出一个协程
            auto it = m_coroutines.begin();
            while(it != m_coroutines.end()){
                if(-1 == it->first || thread_id() == it->first){
                    c = it->second;
                    m_coroutines.erase(it++);
                    ++m_activeTaskCount;
                    break;
                }
            }
        }
        // 有属于当前线程的任务
        if(c != nullptr){  
            ++m_activeThreadCount;    
            c->swapIn();
            --m_activeThreadCount;
            --m_activeTaskCount;
        }
        // 如果闲置协程退出，则线程退出
        Coroutine::State state = idleC->getState();
        if(state == Coroutine::TERM || state == Coroutine::ERROR)
            break;
        // 否则，执行空闲任务
        ++m_idleThreadCount;
        idleC->swapIn();
        --m_idleThreadCount;
        continue;
    }
}

void Scheduler::idle(){
    while(!canStop()){
        Coroutine::Yield();
    }
}

bool Scheduler::canStop(){
    Lock lock(m_mutex);
    return m_stopping && (0 == m_activeTaskCount);
}
__END__