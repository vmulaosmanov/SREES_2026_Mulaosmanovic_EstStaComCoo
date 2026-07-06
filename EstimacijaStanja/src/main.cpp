#include "Application.h"
#include <gui/WinMain.h>

int main(int argc, const char* argv[])
{
    Application app(argc, argv);
    app.initEnvironment();
    app.initRegionals("BA");
    return app.run();
}
