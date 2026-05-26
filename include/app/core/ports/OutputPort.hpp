/**
 * @file OutputPort.hpp
 * @brief Interface for data export and result serialization strategies.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <filesystem>
#include <string>

#include "app/core/grid/MapGrid.hpp"
#include "app/core/snapshot/SnapshotManager.hpp"

namespace floodsim {

/**
 * @class OutputPort
 * @brief Abstract interface for simulation output consumers and writers.
 *
 * Intended to run inside its own independent thread context, isolating expensive
 * I/O disk writes or network transfers from the main execution steps.
 */
class OutputPort {
public:
    virtual ~OutputPort() = default;

    /**
     * @brief Main processing loop for the output thread.
     *
     * This method runs in a dedicated thread. It pulls available snapshots
     * from the thread-safe SnapshotManager until the manager signals shutdown.
     *
     * @param snapshot_manager Reference to the active thread-safe snapshot queue.
     * @param output_path Target directory or file path for the output results.
     */
    virtual void Run(SnapshotManager& snapshot_manager,
                     const std::filesystem::path& output_path) = 0;

    /**
     * @brief Retrieves a descriptive identifier for the worker thread.
     * @return The thread name string (e.g., "Out_IdrisiWriter", "Out_NetworkStream").
     */
    [[nodiscard]] virtual std::string GetThreadName() const = 0;

    /**
     * @brief Provides structural and metadata details of the grid before running.
     * @param grid Reference to the structural initial state of the grid.
     * @param dataset_name The identification name of the current dataset.
     * @param start_timestamp The initial time point of the simulation scenario.
     */
    virtual void SetInitConfig(const MapGrid& grid,
                               const std::string& dataset_name,
                               std::chrono::sys_seconds start_timestamp) = 0;
};

} // namespace floodsim
