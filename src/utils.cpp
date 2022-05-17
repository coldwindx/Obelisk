#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include "utils.h"

__OBELISK__

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


__END__