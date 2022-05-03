#pragma once

#include <pthread.h>
#include <semaphore.h>
#include "system.h"
#include "noncopyable.h"

__OBELISK__
class Semaphore : public Noncopyable{
public:
    Semaphore(uint32_t count = 0){
        if(sem_init(&m_semaphore, 0, count))
            throw std::logic_error("sem_init error!");
    }
    ~Semaphore(){
        sem_destroy(&m_semaphore);
    }

    void wait(){
        if(sem_wait(&m_semaphore))
            throw std::logic_error("sem_wait error!");
    }
    void notify(){
        if(sem_post(&m_semaphore))
            throw std::logic_error("sem_post error!");
    }
private:
    Semaphore(const Semaphore&) = delete;
    Semaphore(const Semaphore&&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;

private:
    sem_t m_semaphore;          // Linux下的信号量
};
__END__