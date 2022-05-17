#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fstream>
#include "utils.h"

__OBELISK__

static int __lstat(const char* file, struct stat* st = nullptr) {
    struct stat lst;
    int ret = lstat(file, &lst);
    if(st) {
        *st = lst;
    }
    return ret;
}

static int __mkdir(const char* dirname) {
    if(access(dirname, F_OK) == 0) {
        return 0;
    }
    return mkdir(dirname, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

void FileUtils::ListAllFile(std::vector<std::string>& files
                            , const std::string& path
                            , const std::string& subfix){
    if(0 != access(path.c_str(), 0))
        return; 
    DIR *dir = opendir(path.c_str());
    if(dir == nullptr)
        return;
    struct dirent *dp = nullptr;
    while((dp = readdir(dir)) != nullptr){
        if(DT_DIR == dp->d_type){
            if(0 == strcmp(dp->d_name, ".") || 0 == strcmp(dp->d_name, ".."))
                continue;
            ListAllFile(files, path + "/" + dp->d_name, subfix);
            continue;
        }
        if(DT_REG == dp->d_type){
            std::string filename(dp->d_name);
            if(subfix.empty()){
                files.push_back(path + "/" + filename);
            }else{
                if(filename.size() < subfix.size())
                    continue;
                if(filename.substr(filename.length() - subfix.size()) == subfix){
                    files.push_back(path + "/" + filename);
                }
            }
        }
    }
    closedir(dir);
}

bool FileUtils::IsRunningPidfile(const std::string& pidfile){
    if(__lstat(pidfile.c_str()) != 0) 
        return false;
    std::ifstream ifs(pidfile);
    std::string line;
    if(!ifs || !std::getline(ifs, line)) 
        return false;
    if(line.empty()) 
        return false;
    pid_t pid = atoi(line.c_str());
    if(pid <= 1) 
        return false;
    if(kill(pid, 0) != 0) 
        return false;
    return true;
}

bool FileUtils::Mkdir(const std::string& dirname){
    if(__lstat(dirname.c_str()) == 0) 
        return true;
    
    char* path = strdup(dirname.c_str());
    char* ptr = strchr(path + 1, '/');
    do {
        for(; ptr; *ptr = '/', ptr = strchr(ptr + 1, '/')) {
            *ptr = '\0';
            if(__mkdir(path) != 0) 
                break;
        }
        if(ptr != nullptr) 
            break;
        if(__mkdir(path) != 0) 
            break;
        free(path);
        return true;
    } while(0);
    free(path);
    return false;
}
__END__