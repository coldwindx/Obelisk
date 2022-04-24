#pragma once

#include <memory>
#include <vector>
#include <list>
#include "system.h"
#include "thread/mutex.h"
#include "coroutine.h"

__OBELISK__
/* 协程调度器---线程池 */
class Scheduler{
public:
    typedef std::shared_ptr<Scheduler> ptr;

    Scheduler(size_t threadsize = 1, const std::string& name = "");
    virtual ~Scheduler();

    const std::string& getName() { return m_name; }
    int getActiveThreadCount() const { return m_activeThreadCount; }
    static Scheduler* GetSelf();

    void start();                                   // 开启调度器，初始化线程池
    void stop();                                    // 停止调度器，等待全部任务执行完毕

    void schedule(std::function<void()> func, int threadId = -1);
    void schedule(Coroutine::ptr c, int threadId = -1);
protected:
    void run();                                     // 线程执行逻辑  
    virtual void idle();                            // 线程空跑 
    virtual bool canStop();                         // 线程终止条件
    virtual void tickle(){}                         // 首次添加任务时的触发函数
private:
    Mutex m_mutex;
    std::vector<Thread::ptr> m_threads;             // 线程池      
                                                    // 协程任务池     
    std::list<std::pair<int, Coroutine::ptr> > m_coroutines;         
    std::string m_name;
protected:
    size_t m_threadCount = 0;
    std::atomic<size_t> m_activeThreadCount = {0};  // 繁忙线程数
    std::atomic<size_t> m_idleThreadCount = {0};    // 空闲线程数
    std::atomic<size_t> m_activeTaskCount = {0};    // 需执行任务数
    bool m_stopping = false;
};
__END__