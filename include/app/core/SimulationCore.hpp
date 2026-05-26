/**
 * @file SimulationCore.hpp
 * @brief Defines the primary simulation engine orchestrating the time-stepping loop.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "app/config/Config.hpp"
#include "app/core/grid/MapGrid.hpp"
#include "app/core/ports/InputPort.hpp"
#include "app/core/ports/OutputPort.hpp"
#include "app/core/ports/StateUpdaterPort.hpp"
#include "app/core/snapshot/ChangeList.hpp"
#include "app/core/snapshot/SnapshotManager.hpp"

namespace floodsim {

/**
 * @struct GridIndexViewBox
 * @brief Defines a rectangular bounding box using discrete grid indices (rows and columns).
 */
struct GridIndexViewBox {
    GridIndexType min_col; ///< Minimum column index.
    GridIndexType max_col; ///< Maximum column index.
    GridIndexType min_row; ///< Minimum row index.
    GridIndexType max_row; ///< Maximum row index.
};

/**
 * @class SimulationCore
 * @brief Core simulation engine.
 *
 * Orchestrates the main execution loop, managing time progression, state updates
 * via the configured neural network / physical solver port, snapshot generation,
 * and I/O publishing.
 */
class SimulationCore {
public:
    /**
     * @brief Constructs the simulation engine with its required dependencies.
     *
     * @param state_updater Port responsible for applying state transitions (e.g., ONNX model).
     * @param main_input_source Primary source port for spatial data.
     * @param layers_alternative_input_source Override mapping for specific layers to custom input ports.
     * @param scalars_config Configuration map for scalar initializations.
     * @param outputs Collection of ports to publish simulation results.
     * @param snapshot_manager Manager handling historical state snapshots and rollbacks.
     * @param config The global simulation configuration settings.
     * @param scenario_name The identifier for the current simulation scenario.
     */
    SimulationCore(
        StateUpdaterPort* state_updater,
        InputPort* main_input_source,
        const std::unordered_map<std::string, InputPort*>& layers_alternative_input_source,
        const std::unordered_map<std::string, std::string>& scalars_config,
        std::vector<OutputPort*> outputs,
        SnapshotManager* snapshot_manager,
        const SimulationConfig& config,
        const std::string& scenario_name
    );

    /**
     * @brief Executes the main simulation time-stepping loop.
     *
     * Will continue advancing the simulation state until the configured duration
     * is reached or the stop flag is signaled.
     *
     * @param stop_flag Thread-safe atomic flag to interrupt the execution loop gracefully.
     */
    void Run(const std::atomic<bool>& stop_flag);

private:
    // -------------------------------------------------------------------------
    // Dependencies (Injected via constructor)
    // -------------------------------------------------------------------------
    StateUpdaterPort* state_updater_;
    SnapshotManager* snapshot_manager_;
    InputPort* main_input_source_;
    std::unordered_map<std::string, InputPort*> layers_alternative_input_source_;
    std::unordered_map<std::string, std::string> scalars_config_;
    std::vector<OutputPort*> outputs_;

    // -------------------------------------------------------------------------
    // Simulation parameters
    // -------------------------------------------------------------------------
    std::chrono::sys_seconds start_timestamp_;
    std::chrono::seconds simulation_duration_;
    std::chrono::seconds time_step_;

    // -------------------------------------------------------------------------
    // Identification
    // -------------------------------------------------------------------------
    std::string scenario_name_;

    // -------------------------------------------------------------------------
    // Simulation state (Mutable)
    // -------------------------------------------------------------------------
    MapGrid current_grid_;
    ViewBox view_;
    GridIndexViewBox grid_index_view_;
    Snapshot current_snapshot_;
    ChangeList changes_;

    /**
     * @struct ThreadResult
     * @brief Aggregates the local indices processed by a single parallel thread.
     */
    struct ThreadResult {
        std::vector<GridIndexType> local_indexes;
    };

    /**
     * @brief Processes a horizontal chunk of the grid to compute state changes.
     *
     * Designed to be executed in a multi-threaded context.
     *
     * @param start_row The starting row index of the chunk (inclusive).
     * @param end_row The ending row index of the chunk (exclusive).
     * @param result Output struct aggregating the indices where changes occurred.
     */
    void ProcessChunk(GridIndexType start_row, GridIndexType end_row, ThreadResult& result);

    /**
     * @brief Calculates and updates the discrete grid index view based on the current geographic view.
     */
    void CalculateGridView();

    /**
     * @brief Computes the delta of changes occurring strictly within the current view area.
     */
    void CalculateViewChanges();

    /**
     * @brief Distributes the current simulation state to all configured output ports.
     *
     * @param time The current simulation clock time.
     */
    void PublishCurrentState(std::chrono::sys_seconds time);
};

} // namespace floodsim
