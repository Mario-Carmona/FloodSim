
#include <iostream>
#include <string>
#include <exception>

#include "app/Application.hpp"
#include "app/logging/Logger.hpp"

namespace {

    /**
     * Prints basic usage of the CLI executable.
     */
    void printUsage(const char* programName)
    {
        std::cerr << "Usage: " << programName
            << " --config <config.yaml>\n";
    }

} // anonymous namespace

int main(int argc, char* argv[])
{
    try {
        // -----------------------------
        // Minimum argument parsing
        // -----------------------------
        if (argc != 3 || std::string(argv[1]) != "--config") {
            printUsage(argv[0]);
            return EXIT_FAILURE;
        }

        const std::string configPath = argv[2];

        // -----------------------------
        // Create application
        // -----------------------------
        danasim::Application app(configPath);

        // -----------------------------
        // Run simulation
        // -----------------------------
        return app.run();
    }
    catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << "\n";
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "Fatal error: unknown exception\n";
        return EXIT_FAILURE;
    }
}
