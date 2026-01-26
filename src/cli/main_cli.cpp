/**
 * @file main_cli.cpp
 * @brief Entry point for the Danasim CLI application.
 *
 * This file contains the main function which handles command-line argument parsing,
 * initializes the application logic via the Configuration Manager, and manages
 * top-level exception handling.
 *
 * @version 1.0
 * @date 2026
 * @copyright Copyright (c) 2026 Danasim
 */

#include <iostream>
#include <string>
#include <string_view>
#include <exception>
#include <cstdlib>

#include "app/Application.hpp"

namespace {

    /**
     * @brief Prints basic usage instructions to stderr.
     *
     * @param programName The name of the executable (usually argv[0]).
     */
    void printUsage(std::string_view programName)
    {
        std::cerr << "Usage: " << programName << " --config /path/to/config.yaml\n";
    }

} // anonymous namespace

/**
 * @brief Main execution entry point.
 *
 * Validates input arguments, bootstraps the Application instance using the
 * provided configuration file, and executes the simulation loop.
 *
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line arguments.
 * @return int Returns EXIT_SUCCESS (0) on success, or EXIT_FAILURE (1) on error.
 *
 * @exception std::exception Catches standard exceptions and logs them before exiting.
 * @exception ... Catches unknown exceptions and logs a generic error.
 */
int main(int argc, char* argv[])
{
    try {
        // -----------------------------
        // Argument Parsing (C++20 Style)
        // -----------------------------
        // We expect exactly 3 arguments: [program_name] [--config] [path]
        if (argc != 3) {
            printUsage(argv[0]);
            return EXIT_FAILURE;
        }

        // Use std::string_view for efficient, non-allocating comparison
        const std::string_view flag{ argv[1] };

        if (flag != "--config") {
            printUsage(argv[0]);
            return EXIT_FAILURE;
        }

        const std::string configPath = argv[2];

        // -----------------------------
        // Application Initialization
        // -----------------------------
        // Instantiate the main application logic with the parsed config path
        danasim::Application app(configPath);

        // -----------------------------
        // Simulation Execution
        // -----------------------------
        // app.run() is expected to block until the simulation completes
        return app.run();
    }
    catch (const std::exception& ex) {
        // Top-level guard for standard library and custom derived exceptions
		std::cerr << "Fatal error: " << ex.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        // Fallback for non-standard exceptions
        std::cerr << "Fatal error: unknown exception" << std::endl;
        return EXIT_FAILURE;
    }
}
