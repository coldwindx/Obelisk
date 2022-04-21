#pragma once

#include <memory>
#include <functional>
#include <ucontext.h>
#include "system.h"
#include "thread/thread.h"

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
    void call();                        // 与当前线程的主协程程切换，执行当前协程
    void call(Coroutine* c);            // 与其他线程的c协程切换，执行当前协程
    void back();                        // 切换到后台执行
    void back(Coroutine* c);            // 与其他线程的c协程切换，执行c协程

    uint64_t getId() const { return m_id; }
    State getState() const { return m_state; }
    void setState(const State& state) { m_state = state; }
                                        
    static void SetThis(Coroutine* c);      // 设置当前协程
    static Coroutine::ptr GetSelf();        // 返回当前协程
    static uint64_t GetCoroutineId();       // 返回当前协程ID
    static void Yield(const State& state);  // 协程切换到后台
    static void Yield(const State& state, Coroutine* c);
    static void Yield(Coroutine* c);

    static uint64_t Total();                // 总协程数
    static void run();
private:
    uint64_t m_id = 0;
    uint32_t m_stacksize = 0;
    State m_state = INIT;

    ucontext_t m_ctx;
    void* m_stack = nullptr;

    std::function<void()> m_callback;

    Coroutine();
};
__END__