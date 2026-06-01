/**
 * @file CheckpointOutput.cpp
 * @brief Implementation of the CheckpointOutput class for simulation state persistence.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/adapters/output/CheckpointOutput.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include <fmt/chrono.h>
#include <fmt/core.h>

#include "app/config/Config.hpp"
#include "app/core/grid/MapGrid.hpp"
#include "logging/Logger.hpp"
#include "app/exception/Exception.hpp"

namespace floodsim::app::adapters::output {

CheckpointOutput::CheckpointOutput(const io::formats::file::FileStaticFormat& static_format)
    : static_format_(static_format)
    , water_depth_writer_(io::writers::file::FileStaticWriterFactory::Create(static_format_, "water_depth")) {

    if (!water_depth_writer_) {
        LOG_ERROR("Failed to create StaticWriter for water_depth");
        throw floodsim::app::exception::FloodSimException("CheckpointOutput: water_depth_writer_ is null upon creation.");
    }
}

void CheckpointOutput::Run(core::snapshot::SnapshotManager& snapshot_manager,
    const std::filesystem::path& output_path) {

    if (output_path.empty()) {
        LOG_ERROR("Provided output path is empty.");
        throw floodsim::app::exception::FloodSimException("CheckpointOutput: output_path cannot be empty.");
    }

    std::filesystem::path checkpoint_output_path = output_path / "checkpoints";

    if (!std::filesystem::exists(checkpoint_output_path)) {
        std::filesystem::create_directories(checkpoint_output_path);
    }

    std::chrono::system_clock::time_point last_processed_step =
        std::chrono::system_clock::time_point::min();

    while (true) {
        try {
            // 1. Block until receiving data
            // We receive the snapshot AND the safety guard
            auto [data, guard] = snapshot_manager.WaitForSnapshot(last_processed_step);
            auto [snapshot, changes] = data;

            // If the guard is null, it means the snapshot_manager has stopped
            if (!guard) {
                LOG_INFO("Stopping CheckpointOutput thread (manager stopped).");
                break;
            }

            LOG_INFO("Consuming snapshot for checkpoint generation.");

            // 2. Processing
            // If this throws an exception, 'guard' is destroyed here and automatically notifies the core.
            SaveSnapshotAsCheckpoint(snapshot, checkpoint_output_path);

            // Update tracking
            last_processed_step = snapshot.Time();
        }
        catch (const std::exception& ex) {
            LOG_ERROR("Error in CheckpointOutput loop: {}", ex.what());
            // Even with an error, the loop continues and the guard releases the Core.
        }
    }
}

void CheckpointOutput::SetInitConfig(const core::grid::MapGrid& grid, const std::string& /* dataset_name */,
                                        std::chrono::sys_seconds /* start_timestamp */) {
    metadata_ = grid.GetMetadata();
}

void CheckpointOutput::SaveSnapshotAsCheckpoint(
        const core::snapshot::Snapshot& snapshot, const std::filesystem::path& checkpoint_output_path) {

    std::string checkpoint_name = fmt::format("checkpoint_{:%FT%T}", snapshot.Time());
    // Replace colons with dashes to avoid issues on certain file systems (e.g., Windows)
    std::replace(checkpoint_name.begin(), checkpoint_name.end(), ':', '-');

    std::filesystem::path current_folder = checkpoint_output_path / checkpoint_name;

    if (!std::filesystem::exists(current_folder)) {
        std::filesystem::create_directories(current_folder);
    }

    water_depth_writer_->Save(current_folder, snapshot.WaterDepth(), metadata_);
}

} // namespace floodsim::app::adapters::output
