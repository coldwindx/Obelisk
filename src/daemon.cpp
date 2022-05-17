#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "log.h"
#include "config.h"
#include "daemon.h"

__OBELISK__

static Logger::ptr g_logger = LOG_SYSTEM();
static ConfigVar<uint32_t>::ptr g_daemon_restart_interval
                    = Config::Lookup("daemon.restart_interval", (uint32_t)5, "daemon restart interval");

// static int real_start(int argc, char** argv, std::function<int(int, char**)> main_cb){
//     return main_cb(argc, argv);
// }

// static int real_daemon(int argc, char** argv, std::function<int(int, char**)> main_cb){
//     ProcessInfo::instance()->parent_id = getpid();
//     ProcessInfo::instance()->parent_start_time = time(0);
//     while(true){
//         pid_t pid = fork();
//         if(0 == pid){
//             // 子进程返回
//             ProcessInfo::instance()->main_id = getpid();
//             ProcessInfo::instance()->main_start_time = time(0);
//             LOG_INFO(g_logger) << "process start pid=" << getpid();
//             return main_cb(argc, argv);
//         }
//         if(pid < 0){
//             LOG_ERROR(g_logger) << "fork fail return=" << pid << " errno=" << errno
//                     << " errstr=" << strerror(errno);
//             return -1;
//         }
//         // 父进程返回
//         int status = 0;
//         waitpid(pid, &status, 0);
//         if(0 == status){
//             LOG_INFO(g_logger) << "child finished pid=" << pid;
//             break;
//         }

//         LOG_ERROR(g_logger) << "child crash pid=" << pid << " status=" << status;
//         ProcessInfo::instance()->restart_count += 1;
//         sleep(g_daemon_restart_interval->getValue());
//     }
//     return 0;
// }

int start_daemon(int argc, char** argv, std::function<int(int, char**)> main_cb, bool is_daemon){
    if(!is_daemon)
        return main_cb(argc, argv);
    daemon(1, 0);
    ProcessInfo::instance()->parent_id = getpid();
    ProcessInfo::instance()->parent_start_time = time(0);
    while(true){
        pid_t pid = fork();
        if(0 == pid){
            // 子进程返回
            ProcessInfo::instance()->main_id = getpid();
            ProcessInfo::instance()->main_start_time = time(0);
            LOG_INFO(g_logger) << "process start pid=" << getpid();
            return main_cb(argc, argv);
        }
        if(pid < 0){
            LOG_ERROR(g_logger) << "fork fail return=" << pid << " errno=" << errno
                    << " errstr=" << strerror(errno);
            return -1;
        }
        // 父进程返回
        int status = 0;
        waitpid(pid, &status, 0);
        if(0 == status){
            LOG_INFO(g_logger) << "child finished pid=" << pid;
            break;
        }
        if(9 == status){
            LOG_INFO(g_logger) << "killed";
            break;
        }
        LOG_ERROR(g_logger) << "child crash pid=" << pid << " status=" << status;
        ProcessInfo::instance()->restart_count += 1;
        sleep(g_daemon_restart_interval->getValue());
    }
    return 0;
}

__END__