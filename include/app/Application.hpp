/**
 * @file Application.hpp
 * @brief Main orchestration class for the FloodSim simulation.
 *
 * This file defines the Application class, which acts as the composition root.
 * It is responsible for loading configuration, wiring together the simulation
 * components (Input, Core, Outputs), and managing the main execution lifecycle.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <atomic>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "app/config/Config.hpp"
#include "app/core/snapshot/SnapshotManager.hpp"
#include "app/core/SimulationCore.hpp"
#include "app/core/ports/OutputPort.hpp"

namespace floodsim {

/**
 * @brief Sets the name of the currently executing thread.
 *
 * Abstraction over OS-specific threading APIs (Windows/pthreads) useful
 * for instrumentation, profiling, and external debuggers.
 *
 * @param name The human-readable name for the thread.
 */
void SetCurrentThreadName(std::string_view name);

/**
 * @brief Retrieves the absolute filesystem path of the running executable.
 *
 * @return std::filesystem::path The location of the binary.
 */
std::filesystem::path GetExecutablePath();

/**
 * @class Application
 * @brief The main entry point and lifecycle manager of the simulation.
 *
 * The Application class adheres to the RAII principle for simulation resources.
 * It parses the configuration, instantiates the simulation core and output
 * ports, and manages the main execution event loop.
 */
class Application {
public:
    /**
     * @brief Constructs the application context and instantiates subsystems.
     *
     * @param config The global parsed configuration object.
     * @param gui_callback Optional callback to emit status logs or progress updates to an external GUI.
     * @throw std::runtime_error If internal dependencies cannot be wired correctly.
     */
    explicit Application(const Config& config,
        std::function<void(int, const std::string&)> gui_callback = nullptr);

    /**
     * @brief Destructor. Ensures all background worker threads are gracefully requested to stop and joined.
     */
    ~Application();

    // Disable copy semantics to prevent unsafe duplicate ownership of the composition root
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Disable move semantics as relocating the application context with active threads is unsafe
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    /**
     * @brief Executes the main simulation time-stepping loop.
     *
     * This method blocks the main thread until the entire simulation finishes,
     * a fatal exception occurs, or cancellation is explicitly requested.
     *
     * @return int Exit code (0 for success, non-zero for failure).
     */
    [[nodiscard]] int Run();

    /**
     * @brief Public thread-safe method intended to be invoked by the GUI or signal handlers to stop execution.
     */
    void Stop();

private:
    /// Atomic flag monitored continuously by execution loops to catch interrupt requests.
    std::atomic<bool> stop_requested_{ false };

    Config config_;                                      ///< The application configuration context.
    std::unique_ptr<SimulationCore> core_;               ///< The central simulation computation engine.
    std::unique_ptr<SnapshotManager> snapshot_manager_;  ///< Pipeline manager handling state capture buffers.
    std::vector<std::unique_ptr<OutputPort>> outputs_;   ///< Set of registered active data sinks.

    /**
     * @brief Asynchronous worker thread pool for concurrent serialization of output ports.
     * @note C++20 std::jthread automates join-on-destruction semantics for increased exception safety.
     */
    std::vector<std::jthread> output_threads_;

    std::filesystem::path output_path_;                  ///< Destination base path for files generation.

    /**
     * @brief Spawns individual background worker loops for each registered output port.
     */
    void StartOutputThreads();

    /**
     * @brief Signals output worker loops to terminate execution and blocks until all threads join.
     */
    void StopOutputThreads();
};

} // namespace floodsim
