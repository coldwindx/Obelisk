#pragma once

#include <memory>
#include <functional>
#include <ucontext.h>
#include "system.hpp"
#include "thread.h"

__OBELISK__
/* 协程
    每个线程拥有一个主协程，子协程的创建均由主协程控制
 */
class Coroutine : public std::enable_shared_from_this<Coroutine>{
public:
    typedef std::shared_ptr<Coroutine> ptr;

    enum State{
        //初始化，暂停，执行中，结束，可执行，异常
        INIT, HOLD, EXEC, TERM, READY, ERROR
    };

    Coroutine(std::function<void()> callback, size_t stacksize = 0);
    ~Coroutine();
                                        // 重置协程函数，并重置状态
    void reset(std::function<void()> callback);
    void swapIn();                      // 协程换入，主协程换出
    void swapIn(Coroutine * c);         // 协程换入，主协程换出
    void swapOut();                     // 协程换出

    uint64_t getId() const { return m_id; }
    State getState() const { return m_state; }
    void setState(const State& state) { m_state = state; }
                                        
    static void SetThis(Coroutine* c);              // 设置当前协程
    static Coroutine::ptr GetSelf();                // 返回当前协程
    static uint64_t GetCoroutineId();               // 返回当前协程ID
    static void Yield(const State& state = HOLD);   // 协程让出

    static uint64_t Total();                        // 总协程数
    static void run();
private:
    uint64_t m_id = 0;                      // 协程ID
    uint32_t m_stacksize = 0;               // 协程运行栈大小
    State m_state = INIT;                   // 协程状态

    ucontext_t m_ctx;
    ucontext_t* m_recCtx = nullptr;
    void* m_stack = nullptr;

    std::function<void()> m_callback;

    Coroutine();
};
__END__