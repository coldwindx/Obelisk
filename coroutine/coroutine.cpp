#include "system.h"
#include <atomic>
#include "config/config.h"
#include "log.h"
#include "coroutine_macro.h"
#include "coroutine.h"

__OBELISK__

static Logger::ptr g_logger = LOG_SYSTEM();

static std::atomic<uint64_t> s_coroutine_id(0);
static std::atomic<uint64_t> s_coroutine_total(0);
                                    // 当前正在执行的协程
static thread_local Coroutine* t_coroutine = nullptr;   
                                    // 当前线程的主协程
static thread_local Coroutine::ptr t_thread_coroutine = nullptr;
                                    // 协程栈空间
static ConfigVar<uint32_t>::ptr g_coroutine_stack_size = 
        Config::lookup<uint32_t>("coroutine.stack_size", 1024 * 1024, "coroutine stack size");

class MemoryAllocator{
public:
    static void* Malloc(size_t size){
        return malloc(size);
    }
    static void Free(void* vp, size_t size){
        return free(vp);
    }
};

Coroutine::Coroutine(){
    m_state = EXEC;
    SetThis(this);
    // 获取当前线程的上下文
    if(getcontext(&m_ctx))
        OBELISK_ASSERT2(false, "getcontext");
    ++s_coroutine_total;

    LOG_DEBUG(g_logger) << "Coroutine::Coroutine";
}

Coroutine::Coroutine(std::function<void()> callback, size_t stacksize)
    : m_id(++s_coroutine_id), m_callback(callback) {
        ++s_coroutine_total;
        if(0 == (m_stacksize = stacksize)) 
            m_stacksize = g_coroutine_stack_size->getValue();
        
        m_stack = MemoryAllocator::Malloc(m_stacksize);
        if(getcontext(&m_ctx))
            OBELISK_ASSERT2(false, "getcontext");
        
        m_ctx.uc_link = nullptr;
        m_ctx.uc_stack.ss_sp = m_stack;
        m_ctx.uc_stack.ss_size = m_stacksize;

        makecontext(&m_ctx, &Coroutine::run, 0);
        LOG_DEBUG(g_logger) << "Coroutine::Coroutine id=" << m_id;
}

Coroutine::~Coroutine(){
    --s_coroutine_total;
    if(m_stack){
        OBELISK_ASSERT(m_state == TERM || m_state == INIT || m_state == ERROR);
        MemoryAllocator::Free(m_stack, m_stacksize);
       LOG_DEBUG(g_logger) << "Coroutine::~Coroutine id=" << m_id;
        return;
    }
    // 没有栈空间，说明当前为主协程
    OBELISK_ASSERT(!m_callback);
    OBELISK_ASSERT(m_state == EXEC);

    Coroutine* cor = t_coroutine;
    if(cor == this) SetThis(nullptr);
    LOG_DEBUG(g_logger) << "Coroutine::~Coroutine id=" << m_id;
}
// 重置协程函数，并重置状态
void Coroutine::reset(std::function<void()> callback){
    OBELISK_ASSERT(m_stack);
    OBELISK_ASSERT(m_state == TERM || m_state == INIT || m_state == ERROR);
    m_callback = callback;
    if(getcontext(&m_ctx))
        OBELISK_ASSERT2(false, "getcontext");
    
    m_ctx.uc_link = nullptr;
    m_ctx.uc_stack.ss_sp = m_stack;
    m_ctx.uc_stack.ss_size = m_stacksize;

    makecontext(&m_ctx, &Coroutine::run, 0);
    m_state = INIT;
}

void Coroutine::swapIn(){
    SetThis(this);
    m_state = EXEC;
    m_recCtx = nullptr;
    swapcontext(&t_thread_coroutine->m_ctx, &m_ctx);
}

void Coroutine::swapIn(Coroutine * c){
    SetThis(this);
    m_state = EXEC;
    m_recCtx = &c->m_ctx;
    swapcontext(&c->m_ctx, &m_ctx);
}

void Coroutine::swapOut(){
    SetThis(this);
    if(m_recCtx)
        swapcontext(&m_ctx, m_recCtx);
    else
        swapcontext(&m_ctx, &t_thread_coroutine->m_ctx);
}

void Coroutine::SetThis(Coroutine* c){
    t_coroutine = c;
}

Coroutine::ptr Coroutine::GetSelf(){
    if(t_coroutine)
        return t_coroutine->shared_from_this();
    Coroutine::ptr mainCoroutine(new Coroutine);
    OBELISK_ASSERT(t_coroutine == mainCoroutine.get());
    t_thread_coroutine = mainCoroutine;
    return t_coroutine->shared_from_this();
}

uint64_t Coroutine::GetCoroutineId(){
    if(t_coroutine)
        return t_coroutine->getId();
    return 0;
}

// 当前协程切换到后台
void Coroutine::Yield(const State& state){
    Coroutine::ptr cur = GetSelf();
    cur->m_state = state;
    cur->swapOut();
}    

uint64_t Coroutine::Total(){
    return s_coroutine_total;
}

void Coroutine::run(){
    Coroutine::ptr cur = GetSelf();
    OBELISK_ASSERT(cur);
    try{
        cur->m_callback();
        cur->m_callback = nullptr;
        cur->m_state = TERM;
    }catch(std::exception& e){
        cur->m_state = ERROR;
        LOG_ERROR(g_logger) << "coroutine except: " << e.what();
    }catch(...){
        cur->m_state = ERROR;
        LOG_ERROR(g_logger) << "coroutine except";
    }
    // 换回主协程
    auto p = cur.get();
    cur.reset();                // 引用计数减一，防止引用计数器只增不减，
    p->swapOut();               // 无法触发析构函数

    OBELISK_ASSERT2(false, "never reach");
}

__END__