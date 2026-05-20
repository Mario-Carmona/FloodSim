
#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "app/core/ports/OutputPort.hpp"
#include "app/io/writers/file/static/FileStaticWriterFactory.hpp"

namespace danasim {

    class CheckpointOutput : public OutputPort {
    public:
        CheckpointOutput(const FileStaticFormat& staticFormat);
        ~CheckpointOutput();

        void run(SnapshotManager& snapshotManager, const std::filesystem::path& outputPath) override;

        void setInitConfig(const MapGrid& grid, const std::string& datasetName, std::chrono::sys_seconds startTimestamp) override;

        std::string getThreadName() const override { return "Out_Checkpoint"; }

    private:
        void saveSnapshotAsCheckpoint(const Snapshot& snapshot, const std::filesystem::path& checkpointOutputPath);

        FileStaticFormat staticFormat_;
		std::unique_ptr<StaticWriter> waterDepthWriter_;
        GridMetadata metadata_;
    };

} // namespace danasim
