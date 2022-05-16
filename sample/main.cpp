#include <iostream>
#include "log.h"
#include "config.h"
#include "daemon.h"
#include "iomanager.h"
#include "timer.h"

using namespace std;
using namespace obelisk;

static Logger::ptr g_logger = LOG_SYSTEM();
Timer::ptr timer;
int server_main(int argc, char **argv){
    IOManager iom(2);
    timer = iom.addTimer(1000, [timer](){
        LOG_INFO(g_logger) << "onTimer";
        static int count = 0;
        if(++count > 10)
            timer->cancel();
    }, true);
}

int main(int argc, char **argv){
    return start_daemon(argc, argv, server_main, argc != 1);
}