#include <dlfcn.h>
#include "hook.h"
#include "coroutine/coroutine.h"
#include "coroutine/iomanager.h"

__OBELISK__

static thread_local bool t_hook_enable = false;

#define HOOK_FUN(XX) \
    XX(sleep)   \
    XX(usleep)  

void hook_init(){
    static bool is_inited = false;
    if(is_inited) return;

#define XX(name) name ## _f = (name ##  _fun)dlsym(RTLD_NEXT, #name);
    HOOK_FUN(XX)
#undef XX
}

struct HookIniter{
    HookIniter(){
        hook_init();
    }
};

static HookIniter s_hook_initer;

bool is_hook_enable(){
    return t_hook_enable;
}

void set_hook_enable(bool flag){
    t_hook_enable = flag;
}

__END__

extern "C"{
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX)
#undef XX
    unsigned int sleep(unsigned int seconds){
        if(!obelisk::t_hook_enable)
            return sleep_f(seconds);
        obelisk::Coroutine::ptr c = obelisk::Coroutine::GetSelf();
        obelisk::IOManager* manager = obelisk::IOManager::GetSelf();
       // manager->addTimer(seconds * 1000, std::bind(&obelisk::IOManager::schedule, manager, c));
        manager->addTimer(seconds * 1000, [manager, c](){manager->schedule(c, obelisk::thread_id());});
        obelisk::Coroutine::Yield();
        return 0;
    }

    int usleep(useconds_t usec){
        if(!obelisk::t_hook_enable)
            return usleep_f(usec);
        obelisk::Coroutine::ptr c = obelisk::Coroutine::GetSelf();
        obelisk::IOManager* manager = obelisk::IOManager::GetSelf();
        //manager->addTimer(usec / 1000, std::bind(&obelisk::IOManager::schedule, manager, c));
        manager->addTimer(usec / 1000, [manager, c](){manager->schedule(c, obelisk::thread_id());});
        obelisk::Coroutine::Yield();
        return 0;
    }
}
