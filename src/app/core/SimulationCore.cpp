/**
 * @file SimulationCore.cpp
 * @brief Implementation of the primary simulation engine.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/core/SimulationCore.hpp"

#include <algorithm>
#include <cmath>
#include <thread>

#include <fmt/chrono.h>
#include <GeographicLib/UTMUPS.hpp>

#include "app/core/snapshot/Snapshot.hpp"
#include "logging/Logger.hpp"

namespace floodsim::app::core {

SimulationCore::SimulationCore(
    ports::StateUpdaterPort* state_updater,
    ports::InputPort* main_input_source,
    const std::unordered_map<std::string, ports::InputPort*>& layers_alternative_input_source,
    const std::unordered_map<std::string, std::string>& scalars_config,
    std::vector<ports::OutputPort*> outputs,
    snapshot::SnapshotManager* snapshot_manager,
    const config::SimulationConfig& config,
    const std::string& scenario_name
)
    : state_updater_(state_updater)
    , main_input_source_(main_input_source)
    , layers_alternative_input_source_(layers_alternative_input_source)
    , scalars_config_(scalars_config)
    , outputs_(std::move(outputs))
    , snapshot_manager_(snapshot_manager)
    , start_timestamp_(config.start_timestamp)
    , simulation_duration_(config.duration)
    , time_step_(config.time_step)
    , scenario_name_(scenario_name)
    , view_(config.view_box)
{
    // Initialize primary grid state
    current_grid_.Load(
        state_updater_->GetModelParamsInfo(),
        main_input_source_,
        layers_alternative_input_source_,
        scalars_config_,
        time_step_,
        start_timestamp_
    );

    CalculateGridView();

    state_updater_->Initialize(current_grid_);

    for (auto& output : outputs_) {
        output->SetInitConfig(current_grid_, main_input_source_->GetDatasetName(), start_timestamp_);
    }
}

void SimulationCore::Run(const std::atomic<bool>& stop_flag) {
    LOG_INFO("Simulation started");

    std::chrono::sys_seconds current_time = start_timestamp_;
    std::chrono::sys_seconds finish_timestamp = current_time + simulation_duration_;

    current_snapshot_.Set(current_time, current_grid_);

    while (current_time < finish_timestamp) {
        // --- STEP 1: LOAD EXTERNAL DATA ---
        // Reads from data sources (e.g., HDF5) and updates dynamic layers (like Rainfall)
        current_grid_.UpdateDynamicLayers(current_time);

        auto start = std::chrono::high_resolution_clock::now();

        StepType steps_taken = 0;
        StepType total_steps_batch = snapshot_manager_->EveryNSteps();

        // --- STEP 2: SOLVER/NN EXECUTION ---
        for (StepType i = 0; i < total_steps_batch; ++i) {
            state_updater_->Step(current_grid_);
            steps_taken++;

            if (stop_flag.load(std::memory_order_relaxed)) {
                break; // Gracefully exit the loop
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        LOG_INFO("Model Execution time: {}s", elapsed.count());
            
        // Advance time accurately based on the actual steps processed
        current_time += (steps_taken * time_step_);

        if (stop_flag.load(std::memory_order_relaxed)) {
            LOG_WARN("Simulation halted early by user request.");
            break;
        }

        // --- STEP 3: OUTPUT ---
        PublishCurrentState(current_time);
    }

    LOG_INFO("Simulation finished");
}

void SimulationCore::PublishCurrentState(std::chrono::sys_seconds time) {
    // 1. Wait for consumers to release the previous buffer
    snapshot_manager_->WaitForReady();

    // 2. Compute spatial differences (deltas) for active regions
    CalculateViewChanges();
    current_snapshot_.Set(time, current_grid_);

    // 3. Publish the new state and changes
    snapshot_manager_->Publish(&current_snapshot_, &changes_);

    LOG_INFO("Published snapshot at time {:%FT%T}", time);
}

void SimulationCore::CalculateGridView() {
    if (view_.use_full_grid) {
        grid_index_view_.min_col = 0u;
        grid_index_view_.max_col = current_grid_.GetCols();

        grid_index_view_.min_row = 0u;
        grid_index_view_.max_row = current_grid_.GetRows();
    }
    else {
        grid::GridViewBox grid_view{
            .south_west = current_grid_.TransformViewPoint(view_.south_west, current_grid_.GetCrs()),
            .north_east = current_grid_.TransformViewPoint(view_.north_east, current_grid_.GetCrs())
        };

        grid_index_view_.min_col = static_cast<GridIndexType>(std::floor((grid_view.south_west.x - current_grid_.GetMapOriginX()) / current_grid_.GetCellSize()));
        grid_index_view_.max_col = static_cast<GridIndexType>(std::ceil((grid_view.north_east.x - current_grid_.GetMapOriginX()) / current_grid_.GetCellSize()));

        grid_index_view_.min_col = std::clamp(grid_index_view_.min_col, static_cast<GridIndexType>(0), current_grid_.GetCols());
        grid_index_view_.max_col = std::clamp(grid_index_view_.max_col, static_cast<GridIndexType>(0), current_grid_.GetCols());

        grid_index_view_.min_row = static_cast<GridIndexType>(std::floor((current_grid_.GetMapOriginY() - grid_view.north_east.y) / current_grid_.GetCellSize()));
        grid_index_view_.max_row = static_cast<GridIndexType>(std::ceil((current_grid_.GetMapOriginY() - grid_view.south_west.y) / current_grid_.GetCellSize()));

        grid_index_view_.min_row = std::clamp(grid_index_view_.min_row, static_cast<GridIndexType>(0), current_grid_.GetRows());
        grid_index_view_.max_row = std::clamp(grid_index_view_.max_row, static_cast<GridIndexType>(0), current_grid_.GetRows());
    }
}

void SimulationCore::CalculateViewChanges() {
    // 1. Thread preparation
    std::vector<std::thread> threads;
    unsigned int hw_threads = std::thread::hardware_concurrency();
    unsigned int num_threads = (hw_threads > 4) ? hw_threads - 1 : std::max(1u, hw_threads);

    std::vector<ThreadResult> results(num_threads);

    // 2. Distribute workload (rows) among threads
    GridIndexType total_view_rows = grid_index_view_.max_row - grid_index_view_.min_row;
    GridIndexType rows_per_thread = total_view_rows / num_threads;
    GridIndexType remainder = total_view_rows % num_threads;

    GridIndexType current_start_row = grid_index_view_.min_row;

    // 3. Launch threads (Fork)
    for (unsigned int i = 0; i < num_threads; ++i) {
        GridIndexType count = rows_per_thread + (i < remainder ? 1 : 0);
        GridIndexType end_row = current_start_row + count;

        if (count > 0) {
            threads.emplace_back(&SimulationCore::ProcessChunk, this,
                current_start_row,
                end_row,
                std::ref(results[i])
            );
        }
        current_start_row = end_row;
    }

    // 4. Wait for threads to complete (Join)
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    // 5. Merge Results (Reduce)

    // Calculate total size beforehand to execute a single reservation
    size_t total_changes = 0;
    for (const auto& res : results) {
        total_changes += res.local_indexes.size();
    }

    changes_.indexes.clear();
    changes_.indexes.reserve(total_changes);

    // Append data contiguously
    for (const auto& res : results) {
        changes_.indexes.insert(changes_.indexes.end(), res.local_indexes.begin(), res.local_indexes.end());
    }
}

void SimulationCore::ProcessChunk(GridIndexType start_row, GridIndexType end_row, ThreadResult& result) {
    // Pre-allocate local memory to avoid constant reallocations.
    // Assuming roughly a 5% change density initially to preserve RAM.
    size_t estimated_changes = static_cast<size_t>((end_row - start_row) * (grid_index_view_.max_col - grid_index_view_.min_col) * 0.05);
    result.local_indexes.reserve(estimated_changes);

    // Single-pass check over the region of interest
    for (GridIndexType y = start_row; y < end_row; ++y) {
        size_t row_offset = static_cast<size_t>(y * current_grid_.GetCols());
        const int8_t* curr_row = current_grid_.GetLayer<int8_t>("flood_risk")->GetData().data() + row_offset;
        const int8_t* prev_row = current_snapshot_.FloodRisk().data() + row_offset;

        // Verify elements inside the mapped view bounds
        for (GridIndexType x = grid_index_view_.min_col; x < grid_index_view_.max_col; ++x) {
            if (prev_row[x] != curr_row[x]) {
                result.local_indexes.push_back(y * current_grid_.GetCols() + x);
            }
        }
    }
}

} // namespace floodsim::app::core
