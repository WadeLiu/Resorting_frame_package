#include "App.h"

int main (int argc, char* argv[])
{
    App* app = new App("assignment.jigentec.com", 49152);
    app->exec();

    printf("Completed\n");

    if (app != nullptr)
    {
        delete app;
    }

    return 0;
}
