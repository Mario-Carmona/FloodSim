/**
 * \file mainCLI.cpp
 * \brief Entry point for the Danasim CLI application.
 *
 * This file contains the main function which handles command-line argument parsing,
 * initializes the application logic via the Configuration Manager, and manages
 * top-level exception handling.
 *
 * \version 1.0
 * \date 2026
 * \copyright Copyright (c) 2026 FloodSim
 */

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

#include "app/Application.hpp"
#include "app/config/ConfigLoader.hpp"

namespace {

    /**
     * \brief Prints basic usage instructions to standard error.
     * \param program_name The name of the executable (usually argv[0]).
     */
    void PrintUsage(std::string_view program_name) {
        std::cerr << "Usage: " << program_name << " [--config /path/to/config.yaml]\n"
            << "If no arguments are provided, the configuration path will be prompted interactively.\n";
    }

}  // namespace

/**
 * \brief Main execution entry point.
 *
 * Validates input arguments, bootstraps the Application instance using the
 * provided configuration file, and executes the simulation loop.
 *
 * \param argc The number of command-line arguments.
 * \param argv The array of command-line arguments.
 * \return EXIT_SUCCESS (0) on successful execution, EXIT_FAILURE (1) otherwise.
 */
int main(int argc, char* argv[]) {
    try {
        std::string config_input_str;

        // 1. Command-line Argument Parsing
        if (argc == 3 && std::string(argv[1]) == "--config") {
            config_input_str = argv[2];
        }
        else if (argc == 1) {
            std::cout << "Enter the path to the configuration file (.yaml): ";
            if (!std::getline(std::cin, config_input_str) || config_input_str.empty()) {
                std::cerr << "[Error] No configuration path provided. Aborting.\n";
                return EXIT_FAILURE;
            }
        }
        else {
            PrintUsage(argv[0]);
            return EXIT_FAILURE;
        }

        // 2. Path Normalization and Validation
        // Converts any relative path into a clean, absolute system path.
        std::filesystem::path absolute_config_path =
            std::filesystem::absolute(config_input_str).lexically_normal();

        if (!std::filesystem::exists(absolute_config_path)) {
            std::cerr << "[Fatal Error] Configuration file not found at: "
                << absolute_config_path << "\n";
            return EXIT_FAILURE;
        }

        std::cout << "Loading configuration from: " << absolute_config_path << "\n";

        // 3. Application Initialization
        danasim::Application app(danasim::ConfigLoader::load(absolute_config_path));

        // 4. Simulation Execution
        // app.run() is expected to block until the simulation completes.
        return app.run();

    }
    catch (const std::exception& ex) {
        // Top-level guard for standard library and custom derived exceptions
        std::cerr << "[Fatal Error] Exception caught: " << ex.what() << "\n";
        return EXIT_FAILURE;
    }
    catch (...) {
        // Fallback for non-standard exceptions
        std::cerr << "[Fatal Error] An unknown critical exception occurred.\n";
        return EXIT_FAILURE;
    }
}
