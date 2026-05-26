/**
 * @file CheckpointOutput.hpp
 * @brief Output port adapter for saving simulation checkpoints.
 *
 * Handles the continuous listening for new simulation snapshots and
 * serializes them into persistent storage to allow for simulation resumes
 * or offline analysis.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>

#include "app/core/ports/OutputPort.hpp"
#include "app/io/writers/file/static/FileStaticWriterFactory.hpp"

namespace floodsim {

/**
 * @class CheckpointOutput
 * @brief Handles saving simulation states as persistent checkpoints.
 */
class CheckpointOutput : public OutputPort {
public:
    /**
     * @brief Constructs a new Checkpoint Output instance.
     *
     * @param static_format The format configuration for the static files.
     * @throws std::runtime_error If the underlying writer fails to instantiate.
     */
    explicit CheckpointOutput(const FileStaticFormat& static_format);

    ~CheckpointOutput() = default;

    /**
     * @brief Main execution loop for the checkpoint output thread.
     *
     * Listens to the snapshot manager and processes incoming data by saving
     * snapshots to the designated checkpoint directory.
     *
     * @param snapshot_manager Reference to the manager providing simulation snapshots.
     * @param output_path The root directory where checkpoints will be saved.
     * @throws std::invalid_argument If the provided output_path is empty.
     */
    void Run(SnapshotManager& snapshot_manager, const std::filesystem::path& output_path) override;

    /**
     * @brief Initializes the output port with base grid metadata.
     *
     * @param grid The simulation map grid.
     * @param dataset_name The name of the current dataset (unused in this adapter).
     * @param start_timestamp The initial time of the simulation (unused in this adapter).
     */
    void SetInitConfig(const MapGrid& grid, const std::string& dataset_name,
        std::chrono::sys_seconds start_timestamp) override;

    /**
     * @brief Retrieves the identifier name for this thread.
     *
     * @return std::string The predefined thread name.
     */
    std::string GetThreadName() const override { return "Out_Checkpoint"; }

private:
    /**
     * @brief Saves a single snapshot as a structured checkpoint on disk.
     *
     * Creates a timestamped directory and delegates data writing to the configured writer.
     *
     * @param snapshot The simulation snapshot to save.
     * @param checkpoint_output_path The base directory for all checkpoints.
     * @throws std::filesystem::filesystem_error If directory creation fails.
     */
    void SaveSnapshotAsCheckpoint(const Snapshot& snapshot,
        const std::filesystem::path& checkpoint_output_path);

    FileStaticFormat static_format_;
    std::unique_ptr<StaticWriter> water_depth_writer_;
    GridMetadata metadata_;
};

} // namespace floodsim
