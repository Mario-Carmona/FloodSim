
#include "app/Application.hpp"
#include <thread>

int main(int argc, char** argv)
{
    // GUI framework init here
    // QApplication appQt(argc, argv);

    danasim::Application app("config.yaml");

    std::thread simulationThread([&app]() {
        app.run();
    });

    danasim::setCurrentThreadName("GUI");

    // appQt.exec();  // GUI event loop

    simulationThread.join();
    return 0;
}
