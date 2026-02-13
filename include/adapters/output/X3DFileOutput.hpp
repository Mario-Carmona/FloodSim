
#pragma once

#include <filesystem>
#include <string>

#include "core/ports/OutputPort.hpp"

namespace danasim {

    /**
     * @brief Writes simulation snapshots to X3D files.
     */
    class X3DFileOutput : public OutputPort {
    public:
        explicit X3DFileOutput(const std::filesystem::path& outputDirectory);

        void run(SnapshotManager& snapshotManager, const std::filesystem::path& outputPath) override;

        void setGrid(const MapGrid& grid) override;

        std::string getThreadName() const override { return "Out_X3D"; }

    private:
        std::filesystem::path outputDirectory_;
    };

} // namespace danasim
