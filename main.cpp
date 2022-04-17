#include <iostream>
#include <string>
#include "log.h"

using namespace std;
using namespace sun;

int main(){
    LOG_DEBUG(LOG_SYSTEM()) << "there has something...";
    return 0;
}