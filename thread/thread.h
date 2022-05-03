#pragma once

#include <pthread.h>
#include <memory>
#include <functional>
#include "../system.h"
#include "semaphore.h"

__OBELISK__

using Callback = std::function<void()>; 

class Thread : public Noncopyable{
public:
    typedef std::shared_ptr<Thread> ptr;

    Thread(std::function<void()> cb, const std::string& name = "UNKNOW");
    ~Thread();

    pid_t getId() const { return m_id; }
    const std::string& getName() const { return m_name; }

    void join();

    static Thread* GetThis();
    static const std::string& GetName();
    static void SetName(const std::string& name);
private:
    Thread(const Thread&) = delete;
    Thread(const Thread&&) = delete;
    Thread& operator=(const Thread&) = delete;

    static void* run(void * arg);
private:
    pid_t m_id = -1;
    pthread_t m_thread;
    std::function<void()> m_cb;
    std::string m_name;

    Semaphore m_semaphore;          // 用于同步线程创建与执行的信号量，
                                    // 确保线程创建完成后执行
};

__END__