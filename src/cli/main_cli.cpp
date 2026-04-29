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
#include <filesystem>

#include "app/Application.hpp"
#include "cli/ConfigLoader.hpp"

namespace {

    /**
     * @brief Prints basic usage instructions to stderr.
     *
     * @param programName The name of the executable (usually argv[0]).
     */
    void printUsage(std::string_view programName)
    {
        std::cerr << "Usage: " << programName << " [--config /path/to/config.yaml]\n";
        std::cerr << "If no arguments are provided, the configuration path will be prompted interactively.\n";
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
        std::string configInputStr;

        // -----------------------------
        // Argument Parsing & Input
        // -----------------------------
        if (argc == 3) {
            // Se pasaron parámetros por línea de comandos
            const std::string_view flag{ argv[1] };

            if (flag != "--config") {
                printUsage(argv[0]);
                return EXIT_FAILURE;
            }
            configInputStr = argv[2];

        }
        else if (argc == 1) {
            // No se pasaron parámetros -> Modo Interactivo
            std::cout << "--- Danasim Configuration ---\n";
            std::cout << "Current Working Directory: " << std::filesystem::current_path() << "\n";
            std::cout << "Enter the path to the configuration file (.yaml): ";

            std::getline(std::cin, configInputStr);

            if (configInputStr.empty()) {
                std::cerr << "Fatal error: Configuration path cannot be empty.\n";
                return EXIT_FAILURE;
            }
        }
        else {
            // Número inválido de argumentos (ej: solo 2, o más de 3)
            printUsage(argv[0]);
            return EXIT_FAILURE;
        }

        // -----------------------------
        // Path Resolution
        // -----------------------------
        // std::filesystem::absolute usa el CWD automáticamente para resolver rutas relativas.
        // Si la ruta ya es absoluta (ej: C:\configs\app.yaml), la deja intacta.
        std::filesystem::path absoluteConfigPath = std::filesystem::absolute(configInputStr).lexically_normal();

        // Opcional: Validar que el archivo realmente existe antes de pasarlo a la app
        if (!std::filesystem::exists(absoluteConfigPath)) {
            std::cerr << "Fatal error: Configuration file not found at " << absoluteConfigPath << "\n";
            return EXIT_FAILURE;
        }

        std::cout << "Loading configuration from: " << absoluteConfigPath << "\n";

        // -----------------------------
        // Application Initialization
        // -----------------------------
        // Pasamos la ruta absoluta convertida a string (o como std::filesystem::path si tu app lo soporta)
        danasim::Application app(danasim::ConfigLoader::load(absoluteConfigPath));

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
