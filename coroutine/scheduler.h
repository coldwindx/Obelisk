#pragma once

#include <memory>
#include <vector>
#include <list>
#include <map>
#include "system.h"
#include "thread/mutex.h"
#include "coroutine.h"

__OBELISK__
/* 协程调度器---线程池 */
class Scheduler{
public:
    typedef std::shared_ptr<Scheduler> ptr;

    Scheduler(size_t threadsize = 1, bool use_caller = true, const std::string& name = "");
    virtual ~Scheduler();

    const std::string& getName() { return m_name; }

    static Scheduler* GetSelf();
    static Coroutine* GetMainCoroutine();

    void start();
    void stop();

    template<typename CoroutineOrCallback>
    void schedule(CoroutineOrCallback fc, int thread = -1){
        bool need_tickle = false;
        {
            Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread);
        }
        if(need_tickle){
            tickle();
        }
    }

    template<typename InputInerator>
    void schedule(InputInerator begin, InputInerator end){
        bool need_tickle = false;
        {
            Lock lock(m_mutex);
            while(begin != end){
                need_tickle = scheduleNoLock(&*begin) || need_tickle;
            }
        }
        if(need_tickle){
            tickle();
        }
    }
protected:
    void run();                 // 协程调度器的逻辑
    void setThis();
    virtual void tickle();
    virtual bool stopping();    // 返回任务是否执行完成
    virtual void idle();        // 无任务时执行   
private:
    template<typename CoroutineOrCallback>
    bool scheduleNoLock(CoroutineOrCallback fc, int thread){
        bool need_tick = m_coroutines.empty();
        ThreadAndCoroutine ft(fc, thread);
        if(ft.coroutine || ft.callback){
            m_coroutines.push_back(ft);
        }
        return need_tick;
    }
private:
    struct ThreadAndCoroutine{
        int threadId;
        Coroutine::ptr coroutine;
        std::function<void()> callback;

        ThreadAndCoroutine(): threadId(-1) {}

        ThreadAndCoroutine(Coroutine::ptr p, int thr)
            : coroutine(p), threadId(thr) {}

        ThreadAndCoroutine(Coroutine::ptr* p, int thr)
            : threadId(thr) {
                coroutine.swap(*p);
        }
        
        ThreadAndCoroutine(std::function<void()> f, int thr)
            : callback(f), threadId(thr) {}

        ThreadAndCoroutine(std::function<void()>* f, int thr)
            : threadId(thr) {
                callback.swap(*f);
        }

        void reset(){
            threadId = -1;
            coroutine = nullptr;
            callback = nullptr;
        }
    };
private:
    Mutex m_mutex;
    std::vector<Thread::ptr> m_threads;
    std::list<ThreadAndCoroutine> m_coroutines;
    Coroutine::ptr m_mainCoroutine;
    std::string m_name;
protected:
    std::vector<int> m_threadIds;
    size_t m_threadCount = 0;
    std::atomic<size_t> m_activeThreadCount = {0};
    std::atomic<size_t> m_idleThreadCount = {0};// 空闲线程数
    bool m_stopping = true;
    bool m_autostop = false;
    int m_mainThreadId = 0;
};
__END__