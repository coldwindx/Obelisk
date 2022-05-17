#include <iostream>
#include "log.h"
#include "config.h"
#include "daemon.h"
#include "iomanager.h"
#include "env.h"

using namespace std;
using namespace obelisk;

static Logger::ptr g_logger = LOG_SYSTEM();

int main(int argc, char **argv){
    cout << "argc=" << argc << endl;
    Env::instance()->addHelp("s", "start with the terminal");
    Env::instance()->addHelp("d", "run as daemon");
    Env::instance()->addHelp("h", "print help");
    if(!Env::instance()->init(argc, argv)){
        Env::instance()->printHelp();
        return 0;
    }

    cout << "exe=" << Env::instance()->getExe() << endl;
    cout << "cwd=" << Env::instance()->getCwd() << endl;
    cout << "path=" << Env::instance()->getEnv("PATH", "xxx") << endl;

    if(Env::instance()->has("h")){
        Env::instance()->printHelp();
    }
    return 0;
}