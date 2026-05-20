
#include "app/adapters/output/CheckpointOutput.hpp"

#include <stdexcept>
#include <limits>
#include <fmt/chrono.h>

#include "app/config/Config.hpp"
#include "app/core/common/SimulationConstants.hpp"
#include "logging/Logger.hpp"
#include "app/core/grid/MapGrid.hpp"

namespace danasim {

    CheckpointOutput::CheckpointOutput(const FileStaticFormat& staticFormat) 
        : staticFormat_(staticFormat)
        , waterDepthWriter_(FileStaticWriterFactory::create(staticFormat_, "water_depth"))
    {
        
    }

    CheckpointOutput::~CheckpointOutput() {
        
    }

    void CheckpointOutput::run(SnapshotManager& snapshotManager, const std::filesystem::path& outputPath) {
        std::filesystem::path checkpointOutputPath = outputPath / "checkpoints";

        if (!std::filesystem::exists(checkpointOutputPath)) {
            std::filesystem::create_directories(checkpointOutputPath);
        }

        std::chrono::system_clock::time_point lastProcessedStep = std::chrono::system_clock::time_point::min();

        while (true) {
            try {
                // 1. Bloqueo hasta recibir datos
                // Recibimos el snapshot Y el guardia de seguridad
                auto [data, guard] = snapshotManager.waitForSnapshot(lastProcessedStep);
                auto [snapshot, changes] = data;

                // Si el guard es null, significa que snapshotManager se ha detenido
                if (!guard) {
                    LOG_INFO("Stopping thread (manager stopped).");
                    break;
                }

                LOG_INFO("Consuming snapshot");

                // 2. Procesamiento
                // Si esto falla (excepción), 'guard' se destruye aquí y avisa al core automáticamente.
                saveSnapshotAsCheckpoint(snapshot, checkpointOutputPath);

                // Actualizamos tracking
                lastProcessedStep = snapshot.time();

                // AL FINAL DEL SCOPE: 'guard' se destruye -> llama a signalDone()
            }
            catch (const std::exception& ex) {
                LOG_ERROR("Error: {}", ex.what());
                // Incluso con error, el loop continúa y el guard liberó al Core.
                // Si quieres que un error mate el hilo, pon un break aquí.
            }
        }
    }

    void CheckpointOutput::setInitConfig(const MapGrid& grid, const std::string& /* datasetName */, std::chrono::sys_seconds /* startTimestamp */) {
        metadata_ = grid.getMetadata();
    }

    void CheckpointOutput::saveSnapshotAsCheckpoint(const Snapshot& snapshot, const std::filesystem::path& checkpointOutputPath) {

        std::string checkpointName = fmt::format("checkpoint_{:%FT%T}", snapshot.time());
        std::replace(checkpointName.begin(), checkpointName.end(), ':', '-');

        std::filesystem::path currentFolder = checkpointOutputPath / checkpointName;

        std::filesystem::create_directories(currentFolder);

        waterDepthWriter_->save(currentFolder, snapshot.waterDepth(), metadata_);
    }

} // namespace danasim
