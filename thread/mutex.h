#pragma once

#include "system.h"
#include "lock.h"
#include <atomic>

__OBELISK__

class Mutex;
class RWMutex;
class SpinMutex;
class CASMutex;

typedef ScopedLock<Mutex> Lock;
typedef ReadScopedLock<RWMutex> ReadLock;
typedef WriteScopedLock<RWMutex> WriteLock;
typedef ScopedLock<SpinMutex> SpinLock;
typedef ScopedLock<CASMutex> CASLock;

#if _USE_THREAD_
/* 互斥量 */
class Mutex{
public:
    Mutex(){
        pthread_mutex_init(&m_mutex, nullptr);
    }
    ~Mutex(){
        pthread_mutex_destroy(&m_mutex);
    }
    void lock(){
        pthread_mutex_lock(&m_mutex);
    }
    void unlock(){
        pthread_mutex_unlock(&m_mutex);
    }
private:
    pthread_mutex_t m_mutex;
};

/* 读写互斥量 */
class RWMutex{
public:
    RWMutex(){
        pthread_rwlock_init(&m_lock, nullptr);
    }
    ~RWMutex(){
        pthread_rwlock_destroy(&m_lock);
    }
    void rdlock(){
        pthread_rwlock_rdlock(&m_lock);
    }
    void wrlock(){
        pthread_rwlock_wrlock(&m_lock);
    }
    void unlock(){
        pthread_rwlock_unlock(&m_lock);
    }
private:
    pthread_rwlock_t m_lock;
};

/* 自旋锁互斥量 */
class SpinMutex{
public:
    SpinMutex(){
        pthread_spin_init(&m_mutex, 0);
    }
    ~SpinMutex(){
        pthread_spin_destroy(&m_mutex);
    }
    void lock(){
        pthread_spin_lock(&m_mutex);
    }
    void unlock(){
        pthread_spin_unlock(&m_mutex);
    }
private:
    pthread_spinlock_t m_mutex;
};

/* 原子互斥量 */
class CASMutex{
public:
    CASMutex(){
        m_mutex.clear();
    }
    ~CASMutex(){

    }
    void lock(){
        while(std::atomic_flag_test_and_set_explicit(&m_mutex, std::memory_order_acquire)){}
    }
    void unlock(){
        std::atomic_flag_clear_explicit(&m_mutex, std::memory_order_release);
    }
private:
    std::atomic_flag m_mutex;
};

#else 

class Mutex{
public:
    Mutex(){}
    ~Mutex(){}
    void lock(){}
    void unlock(){}
};

class RWMutex{
public:
    RWMutex(){}
    ~RWMutex(){}
    void rdlock(){}
    void wrlock(){}
    void unlock(){}
};

#endif

__END__