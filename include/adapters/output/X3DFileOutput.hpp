
#pragma once

#include <filesystem>
#include <string>

#include "ports/OutputPort.hpp"

namespace danasim {

    /**
     * @brief Writes simulation snapshots to X3D files.
     */
    class X3dFileOutput : public OutputPort {
    public:
        explicit X3dFileOutput();

        void run(SnapshotManager& snapshotManager, const std::filesystem::path& outputPath) override;

        void setGrid(const MapGrid& grid) override;

        std::string getThreadName() const override { return "Out_X3D"; }
    };

} // namespace danasim
