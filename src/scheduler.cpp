#include "coroutine_macro.h"
#include "hook.h"
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
    if(m_tasks.empty()) tickle();
    m_tasks.push_back(Task(func, threadId));
}

void Scheduler::schedule(Coroutine::ptr c, int threadId){
    Lock lock(m_mutex);
    if(m_tasks.empty()) tickle();
    m_tasks.push_back(Task(c, threadId));
}


void Scheduler::run(){
    LOG_DEBUG(g_logger) << "Scheduler::run()";
    set_hook_enable(true);
    t_scheduler = this;

    Coroutine::GetSelf();
    std::list<Coroutine*> pool;                          // 空闲协程池
    Coroutine::ptr idleC(new Coroutine(std::bind(&Scheduler::idle, this)));
    
    auto fetch = [&](Callback cb){
        if(pool.empty()){
            Coroutine* c = new Coroutine(cb);
            return Coroutine::ptr(c, [&](Coroutine* p){ pool.push_back(p); });
        }
        Coroutine* c = pool.front();
        pool.pop_front();
        c->reset(cb);
        return Coroutine::ptr(c, [&](Coroutine* p){ pool.push_back(p); });
    };

    Task task;
    while(true){
        task.reset();
        {
            Lock lock(m_mutex);
            // 取出一个协程
            for(auto it = m_tasks.begin(); it != m_tasks.end(); ++it){
                if(-1 == it->threadId || thread_id() == it->threadId){
                    task = *it;
                    m_tasks.erase(it);
                    ++m_executingTaskCount;
                    break;
                }
            }
        }
        // 为任务分配一个协程
        if(task.callback)
            task.coroutine = fetch(task.callback);
        // 处理协程任务
        if(task.coroutine){
            ++m_activeThreadCount;
            // LOG_DEBUG(g_logger) << "coroutine id=" << task.coroutine->getId() << " swap in";
            task.coroutine->swapIn();
            // LOG_DEBUG(g_logger) << "coroutine id=" << task.coroutine->getId() << " swap out";
            --m_activeThreadCount;
            --m_executingTaskCount;
            if(task.coroutine->getState() == Coroutine::READY)
                this->schedule(task.coroutine, task.threadId);             
            continue;
        }
        // 没有任务时处理闲置任务
        ++m_idleThreadCount;
        idleC->swapIn();
        --m_idleThreadCount;
        Coroutine::State state = idleC->getState();
        // 如果闲置协程退出，则线程退出
        if(state == Coroutine::TERM)
            break;
    }
    // 结束时释放协程队列
    for(auto & p : pool) delete p;
}

void Scheduler::idle(){
    while(!canStop()){
        Coroutine::Yield();
    }
}

bool Scheduler::canStop(){
    Lock lock(m_mutex);
    return m_stopping && (0 == m_executingTaskCount);
}
__END__