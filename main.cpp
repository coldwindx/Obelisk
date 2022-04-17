#include <iostream>
#include <string>
#include "log.h"
#include "config/convert.h"

using namespace std;
using namespace obelisk;

int main(){
    LOG_DEBUG(LOG_SYSTEM()) << "there has something...";
    cout << Convert<string, int>()("92") << endl;
    return 0;
}