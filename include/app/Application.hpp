/**
 * @file Application.hpp
 * @brief Main orchestration class for the Danasim simulation.
 *
 * This file defines the Application class, which acts as the composition root.
 * It is responsible for loading configuration, wiring together the simulation
 * components (Input, Core, Outputs), and managing the main execution lifecycle.
 *
 * @version 1.0
 * @date 2026
 * @copyright Copyright (c) 2026 Danasim
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <thread>

#include "app/config/Config.hpp"

namespace danasim {

    // Forward declarations to reduce compile-time dependencies
    class SimulationCore;
    class SnapshotManager;
    class OutputPort;

    /**
     * @brief Sets the name of the currently executing thread.
     * * Abstraction over OS-specific threading APIs (Windows/pthreads).
     * * @param name The human-readable name for the thread (e.g., for debuggers).
     */
    void setCurrentThreadName(std::string_view name);

    /**
     * @class Application
     * @brief The main entry point and lifecycle manager of the simulation.
     *
     * The Application class adheres to the RAII principle for simulation resources.
     * It parses the configuration, instantiates the simulation core and output
     * ports, and manages the main event loop.
     */
    class Application {
    public:
        /**
         * @brief Constructs the application and loads the configuration.
         * * @param configPath Path to the YAML configuration file.
         * @throw std::runtime_error If configuration loading fails.
         */
        explicit Application(const std::filesystem::path& configPath);

        /**
         * @brief Destructor. Ensures all threads are stopped safely.
         */
        ~Application();

        /**
         * @brief Executes the main simulation loop.
         *
         * This method blocks until the simulation finishes or a fatal error occurs.
         *
         * @return int Exit code (0 for success, non-zero for failure).
         */
        int run();

    private:
        /// The parsed application configuration.
        Config config_;

        /// The central simulation logic engine.
        std::unique_ptr<SimulationCore> core_;

        /// Manager for simulation snapshots (state capture).
        std::unique_ptr<SnapshotManager> snapshotManager_;

        /// Collection of active output ports (sinks).
        std::vector<std::unique_ptr<OutputPort>> outputs_;

        /**
         * @brief Worker threads for output processing.
         * * @note C++20 std::jthread automatically joins on destruction,
         * providing exception safety.
         */
        std::vector<std::jthread> outputThreads_;

        std::filesystem::path outputPath_;

        /**
         * @brief Spawns threads for all registered output ports.
         */
        void startOutputThreads();

        /**
         * @brief Signals output threads to stop and waits for them to join.
         */
        void stopOutputThreads();
    };

} // namespace danasim
