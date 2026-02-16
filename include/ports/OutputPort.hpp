/**
 * @file OutputPort.hpp
 * @brief Interface for data export strategies.
 *
 * @version 1.0
 * @date 2026
 * @copyright Copyright (c) 2026 Danasim
 */

#pragma once

#include <string>
#include <filesystem>

#include "core/snapshot/SnapshotManager.hpp"

namespace danasim {

    /**
     * @interface OutputPort
     * @brief Abstract interface for simulation output consumers.
     *
     * An OutputPort runs in its own thread and consumes snapshots produced
     * by the simulation core.
     */
    class OutputPort {
    public:
        virtual ~OutputPort() = default;

        /**
         * @brief Main execution loop for the output consumer.
         *
         * This method is called once by the Application in a dedicated thread.
         * It should run a loop that waits for data from the SnapshotManager
         * until the simulation stops.
         *
         * @param[in,out] snapshotManager Reference to the thread-safe snapshot queue/manager.
         */
        virtual void run(SnapshotManager& snapshotManager, const std::filesystem::path& outputPath) = 0;

        /**
         * @brief Retrieves the human-readable name for the output thread.
         *
         * Used for logging and debugging purposes.
         *
         * @return std::string The name of the thread (e.g., "Out_MQTT").
         */
        [[nodiscard]]
        virtual std::string getThreadName() const = 0;

        virtual void setGrid(const MapGrid& grid) = 0;
    };

} // namespace danasim
