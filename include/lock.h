#pragma once

#include "system.hpp"

__OBELISK__

/* 局部互斥锁(RAII) */
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

/* 局部读锁(RAII) */
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

/* 局部写锁(RAII) */
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