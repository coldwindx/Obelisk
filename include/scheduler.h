#pragma once

#include <memory>
#include <vector>
#include <list>
#include "system.hpp"
#include "thread.h"
#include "mutex.h"
#include "coroutine.h"

__OBELISK__
/* 协程调度器---线程池 */
class Scheduler{
public:
    typedef std::shared_ptr<Scheduler> ptr;

    Scheduler(size_t threadsize = 1, const std::string& name = "scheduler");
    virtual ~Scheduler();

    const std::string& getName() { return m_name; }
    int getActiveThreadCount() const { return m_activeThreadCount; }
    static Scheduler* GetSelf();

    void start();                                   // 开启调度器，初始化线程池
    void stop();                                    // 停止调度器，等待全部任务执行完毕

    void schedule(Callback func, int threadId = -1);
    void schedule(Coroutine::ptr c, int threadId = -1);
    template<typename Iterator>
    void schedule(Iterator begin, Iterator end, int threadId = -1){
        Lock lock(m_mutex);
        if(m_tasks.empty()) tickle();
        for(; begin != end; ++begin)
            m_tasks.push_back(Task(*begin, threadId));
    }
protected:
    void run();                                     // 线程执行逻辑  
    virtual void idle();                            // 线程空跑 
    virtual bool canStop();                         // 线程终止条件
    virtual void tickle(){}                         // 首次添加任务时的触发函数

    struct Task{
        Task() {}
        Task(Callback  call, int threadId = -1) : callback(call), threadId(threadId) {}
        Task(Coroutine::ptr c, int threadId = -1) : coroutine(c), threadId(threadId) {}
        void reset(){
            threadId = -1;
            callback = nullptr;
            coroutine.reset();
        }
        int threadId = -1;
        Callback callback = nullptr;
        Coroutine::ptr coroutine = nullptr;
    };
private:
    Mutex m_mutex;
    std::vector<Thread::ptr> m_threads;             // 线程池      
    std::list<Task> m_tasks;              // 协程任务队列
    std::string m_name;
protected:
    size_t m_threadCount = 0;
    std::atomic<size_t> m_activeThreadCount = {0};  // 繁忙线程数
    std::atomic<size_t> m_idleThreadCount = {0};    // 空闲线程数
    std::atomic<size_t> m_executingTaskCount = {0}; // 需执行任务数
    bool m_stopping = false;
};
__END__