
#pragma once

#include <filesystem>
#include <string>

#include "app/core/ports/OutputPort.hpp"

namespace danasim {

    /**
     * @brief Writes simulation snapshots to X3D files.
     */
    class X3dFileOutput : public OutputPort {
    public:
        explicit X3dFileOutput();

        void run(SnapshotManager& snapshotManager, const std::filesystem::path& outputPath) override;

        void setInitConfig(const MapGrid& grid, const std::string& datasetName, std::chrono::sys_seconds startTimestamp) override;

        std::string getThreadName() const override { return "Out_X3D"; }
    };

} // namespace danasim
