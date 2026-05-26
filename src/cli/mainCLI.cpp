/**
 * @file mainCLI.cpp
 * @brief Entry point for the FloodSim CLI application.
 *
 * Handles command-line arguments, normalizes configuration paths,
 * and acts as the top-level exception guard for the simulation.
 *
 * @copyright Copyright (c) 2026 FloodSim
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
 * @brief Prints basic usage instructions to the designated output stream.
 * @param program_name The name of the executable (usually argv[0]).
 * @param out The output stream to write to (defaults to std::cout).
 */
void PrintUsage(std::string_view program_name, std::ostream& out = std::cout) {
    out << "Usage:\n"
        << "  " << program_name << " [--config /path/to/config.yaml]\n"
        << "  " << program_name << " [--version]\n"
        << "  " << program_name << " [--help]\n\n"
        << "Options:\n"
        << "  --config <path>   Path to the YAML configuration file.\n"
        << "  --version         Show the application version and exit.\n"
        << "  --help            Show this help message and exit.\n\n"
        << "If no arguments are provided, the configuration path will be prompted interactively.\n";
}

}  // namespace

/**
 * @brief Main execution entry point.
 *
 * Validates input arguments, bootstraps the Application instance using the
 * provided configuration file, and executes the simulation loop.
 *
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line arguments.
 * @return EXIT_SUCCESS (0) on successful execution, EXIT_FAILURE (1) otherwise.
 */
int main(int argc, char* argv[]) {
    try {
        std::string config_input_str;

        // 1. Command-line Argument Parsing
        if (argc == 2 && std::string_view(argv[1]) == "--help") {
            PrintUsage(argv[0], std::cout);
            return EXIT_SUCCESS;
        }

        if (argc == 2 && std::string_view(argv[1]) == "--version") {
            std::cout << "FloodSim version 1.0 (2026)\n";
            return EXIT_SUCCESS;
        }

        if (argc == 3 && std::string_view(argv[1]) == "--config") {
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
            PrintUsage(argv[0], std::cerr);
            return EXIT_FAILURE;
        }

        // 2. Path Normalization and Validation
        std::filesystem::path absolute_config_path;
        try {
            absolute_config_path = std::filesystem::absolute(config_input_str).lexically_normal();
        }
        catch (const std::filesystem::filesystem_error& err) {
            std::cerr << "[Fatal Error] System filesystem error processing path: " << err.what() << "\n";
            return EXIT_FAILURE;
        }

        if (!std::filesystem::exists(absolute_config_path)) {
            std::cerr << "[Fatal Error] Configuration file not found at: " << absolute_config_path << "\n";
            return EXIT_FAILURE;
        }

        std::cout << "Loading configuration from: " << absolute_config_path << "\n";

        // 3. Application Initialization
        // ConfigLoader::Load will throw a ConfigurationException if parsing or validation fails
        floodsim::app::config::Config config = floodsim::app::config::ConfigLoader::Load(absolute_config_path);
        floodsim::app::Application app(std::move(config));

        // 4. Simulation Execution
        return app.Run();

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
