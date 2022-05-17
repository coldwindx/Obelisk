#include "application.h"

int main(int argc, char** argv){
    obelisk::Application app;
    if(app.init(argc, argv))
        return app.run();
    return 0;
}