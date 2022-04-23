#include "log.h"
#include "coroutine_macro.h"
#include "scheduler.h"

__OBELISK__

static Logger::ptr g_logger = LOG_SYSTEM();

static thread_local Scheduler* t_scheduler = nullptr;
static thread_local Coroutine* t_main_coroutine = nullptr;

Scheduler::Scheduler(size_t threadsize, bool use_caller, const std::string& name)
    : m_name(name) {
    OBELISK_ASSERT(threadsize > 0);

    if(use_caller){
        Coroutine::GetSelf();       // 初始化一个主协程
        --threadsize;
        OBELISK_ASSERT(GetSelf() == nullptr);
        t_scheduler = this;

        m_mainCoroutine.reset(new Coroutine(std::bind(&Scheduler::run, this), 0));
        Thread::SetName(m_name);

        t_main_coroutine = m_mainCoroutine.get();
        m_mainThreadId = thread_id();
        m_threadIds.push_back(m_mainThreadId);
    }else{
        m_mainThreadId = -1;
    }
    m_threadCount = threadsize;
}

Scheduler::~Scheduler(){
    OBELISK_ASSERT(m_stopping);
    if(GetSelf() == this)
        t_scheduler = nullptr;
}

Scheduler* Scheduler::GetSelf(){
    return t_scheduler;
}

Coroutine* Scheduler::GetMainCoroutine(){
    return t_main_coroutine;
}

void Scheduler::start(){
    Lock lock(m_mutex);
    if(!m_stopping) return ;
    m_stopping = false;

    OBELISK_ASSERT(m_threads.empty());

    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount; ++i){
        m_threads[i].reset(new Thread(std::bind(&Scheduler::run, this)
                    , m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
    // 协程切换时，锁局部变量没释放，导致run方法拿不到锁，
    lock.unlock();
    if(m_mainCoroutine){
        m_mainCoroutine->swapIn();
        LOG_DEBUG(g_logger) << "call out";
    }
}

void Scheduler::stop(){
    m_autostop = true;
    LOG_DEBUG(g_logger) << "in Scheduler::stop()";
    if(m_mainCoroutine && m_threadCount == 0 
            && (m_mainCoroutine->getState() == Coroutine::TERM 
                    || m_mainCoroutine->getState() == Coroutine::INIT)){
        LOG_INFO(g_logger) << this << " stopped!";
        m_stopping = true;

        if(stopping()) return ;
    }
   // bool exit_on_this_coroutine = false;
    if(m_mainThreadId != -1){
        OBELISK_ASSERT(GetSelf() == this);
    }else{
        OBELISK_ASSERT(GetSelf() != this);
    }
    m_stopping = true;
    for(size_t i = 0; i < m_threadCount; ++i){
        tickle();
    }

    if(m_mainCoroutine){
        tickle();
    }
    // if(exit_on_this_coroutine){

    // }
}

void Scheduler::run(){
    LOG_DEBUG(g_logger) << "Schedule::run()";
   setThis();
   if(thread_id() != m_mainThreadId){
       t_main_coroutine = Coroutine::GetSelf().get();
   }
   Coroutine::ptr idleCoroutine(new Coroutine(std::bind(&Scheduler::idle, this)));
   Coroutine::ptr cbCoroutine;

    ThreadAndCoroutine ft;
   while(true){
       ft.reset();
       bool tickleMe = false;
       {
           // 取出一个协程
           Lock lock(m_mutex);
           auto it = m_coroutines.begin();
           while(it != m_coroutines.end()){
               if(-1 != it->threadId && it->threadId != thread_id()){
                   ++it;
                   tickleMe = true;
                   continue;
               }
               OBELISK_ASSERT(it->coroutine || it->callback);
               if(it->coroutine && it->coroutine->getState() == Coroutine::EXEC){
                   ++it;
                   continue;
               }
               ft = *it;
               m_coroutines.erase(it++);
               ++m_activeThreadCount;
               break;
           }
           tickleMe |= it != m_coroutines.end();
       }
        if(tickleMe)
            tickle();
        if(ft.coroutine && ft.coroutine->getState() != Coroutine::TERM 
                    && ft.coroutine->getState() != Coroutine::ERROR){
            ft.coroutine->swapIn(t_main_coroutine);
            --m_activeThreadCount;

            if(ft.coroutine->getState() == Coroutine::READY){
                schedule(ft.coroutine);
            }else if(ft.coroutine->getState() != Coroutine::TERM
                    && ft.coroutine->getState() != Coroutine::ERROR){
                ft.coroutine->setState(Coroutine::HOLD);
            }
            ft.reset();
        }else if(ft.callback){
            if(cbCoroutine){
                cbCoroutine->reset(ft.callback);
            }else{
                cbCoroutine.reset(new Coroutine(ft.callback));
            }
            ft.reset();

            cbCoroutine->swapIn(t_main_coroutine);
            --m_activeThreadCount;

            if(cbCoroutine->getState() == Coroutine::READY){
                schedule(cbCoroutine);
                cbCoroutine.reset();
            }else if(cbCoroutine->getState() == Coroutine::ERROR
                    || cbCoroutine->getState() == Coroutine::TERM){
                cbCoroutine->reset(nullptr);
            }else {
                cbCoroutine->setState(Coroutine::HOLD);
                cbCoroutine.reset();
            }
        }else{
            if(idleCoroutine->getState() == Coroutine::TERM){
                LOG_INFO(g_logger) << "idle coroutine term";
                break;
            }
            ++m_idleThreadCount;
            idleCoroutine->swapIn(t_main_coroutine);
            -- m_idleThreadCount;
            if(idleCoroutine->getState() != Coroutine::TERM
                    && idleCoroutine->getState() != Coroutine::ERROR){
                idleCoroutine->setState(Coroutine::HOLD);
            }
            
        }
   }
}
void Scheduler::setThis(){
    t_scheduler = this;
}


void Scheduler::tickle(){
    LOG_INFO(g_logger) << "tickle";
}
bool Scheduler::stopping(){
    LOG_INFO(g_logger) << "stopping";
    Lock lock(m_mutex);
    return m_autostop && m_stopping && m_coroutines.empty()
            && m_activeThreadCount == 0;
}
void Scheduler::idle(){
    LOG_INFO(g_logger) << "idle";
    // while(!stopping()) {
    //     Coroutine::Yield(t_main_coroutine);
    // }
}
__END__