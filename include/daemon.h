#pragma once

#include <functional>
#include "system.hpp"

__OBELISK__

class ProcessInfo{
public:
    static ProcessInfo* instance(){
        static ProcessInfo* p = new ProcessInfo();
        return p;
    }
    // 父进程ID
    pid_t parent_id = 0;
    // 主进程ID
    pid_t main_id = 0;
    // 父进程启动时间
    uint64_t parent_start_time = 0;
    // 主进程启动时间
    uint64_t main_start_time = 0;
    // 主进程重启次数
    uint32_t restart_count = 0;
private:
    ProcessInfo(){}
};
/**
 * @brief 启动程序，可以选择使用守护进程
 * @param[in] argc: 参数个数
 * @param[in] argv: 参数数组
 * @param[in] main: 启动函数
 * @param[in] is_daemon: 是否使用守护进程的方式
 */
int start_daemon(int argc, char** argv, std::function<int(int, char**)> main, bool is_daemon);

__END__