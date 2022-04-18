#pragma once

#include "../system.h"

__OBELISK__

/* 互斥信号量 */
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

/* 互斥锁(RAII) */
template<typename T>
struct ScopedLock{
public:
    ScopedLock(T & mutex) : m_mutex(mutex){
        m_mutex.lock();
        m_locked = true;
    }
    ~ScopedLock(){
        unlock();
    }
    void lock(){
        if(!m_locked){
            m_mutex.lock();
            m_locked = true;
        }
    }
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T & m_mutex;
    bool m_locked;
};

/* 读锁(RAII) */
template<typename T>
struct ReadScopedLock{
public:
    ReadScopedLock(T & mutex) : m_mutex(mutex){
        m_mutex.rdlock();
        m_locked = true;
    }
    ~ReadScopedLock(){
        unlock();
    }
    void lock(){
        if(!m_locked){
            m_mutex.rdlock();
            m_locked = true;
        }
    }
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T & m_mutex;
    bool m_locked;
};

/* 写锁(RAII) */
template<typename T>
struct WriteScopedLock{
public:
    WriteScopedLock(T & mutex) : m_mutex(mutex){
        m_mutex.wrlock();
        m_locked = true;
    }
    ~WriteScopedLock(){
        unlock();
    }
    void lock(){
        if(!m_locked){
            m_mutex.wrlock();
            m_locked = true;
        }
    }
    void unlock(){
        if(m_locked){
            m_mutex.unlock();
            m_locked = false;
        }
    }
private:
    T & m_mutex;
    bool m_locked;
};

__END__