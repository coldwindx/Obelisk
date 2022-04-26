#include "coroutine/coroutine_macro.h"
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

    Coroutine::ptr main = Coroutine::GetSelf();
    std::list<Coroutine::ptr> coroutines;

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
        // 有属于当前线程的任务
        if(task.coroutine){
            ++m_activeThreadCount;
            task.coroutine->swapIn(main.get());
            --m_activeThreadCount;
            --m_executingTaskCount;
            if(task.coroutine->getState() == Coroutine::READY)
                this->schedule(task.coroutine, task.threadId);             
            continue;
        }

        if(task.callback){
            if(coroutines.empty())
                task.coroutine = Coroutine::ptr(new Coroutine(task.callback));
            else{
                task.coroutine = coroutines.front();
                coroutines.pop_front();
                task.coroutine->reset(task.callback);
            }
            ++m_activeThreadCount;
            task.coroutine->swapIn(main.get());
            --m_activeThreadCount;
            --m_executingTaskCount;
            Coroutine::State state = task.coroutine->getState();
            if(state == Coroutine::READY)
                this->schedule(task.coroutine, task.threadId);
            if(state == Coroutine::TERM || state == Coroutine::ERROR)
                coroutines.push_back(task.coroutine);
            task.reset();
            continue;
        }

        // 如果闲置协程退出，则线程退出
        if(coroutines.empty())
            task.coroutine = Coroutine::ptr(new Coroutine(std::bind(&Scheduler::idle, this)));
        else{
            task.coroutine = coroutines.front();
            coroutines.pop_front();
            task.coroutine->reset(std::bind(&Scheduler::idle, this));
        }
        ++m_idleThreadCount;
        task.coroutine->swapIn(main.get());
        --m_idleThreadCount;
        Coroutine::State state = task.coroutine->getState();
        if(state == Coroutine::TERM)
            break;
        if(state == Coroutine::HOLD){
            task.coroutine->reset(nullptr);
            coroutines.push_back(task.coroutine);
        }
    }
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