
#include "adapters/output/X3DFileOutput.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "app/logging/Logger.hpp"
#include "adapters/snapshot_serializer/X3DSnapshotSerializer.hpp"

namespace danasim {

    X3DFileOutput::X3DFileOutput(const std::filesystem::path& outputDirectory)
        : outputDirectory_(outputDirectory)
    {
        if (!std::filesystem::exists(outputDirectory_)) {
            std::filesystem::create_directories(outputDirectory_);
        }
    }

    void X3DFileOutput::run(SnapshotManager& snapshotManager)
    {
        uint64_t lastProcessedStep = 0xFFFFFFFFFFFFFFFF; // Empezamos desde antes del paso 0

        while (true) {
            try {
                // 1. Bloqueo hasta recibir datos (RAII Pattern)
                // Obtenemos el snapshot y el 'guard' que gestiona la señalización
                auto [snapshot, guard] = snapshotManager.waitForSnapshot(lastProcessedStep);

                // Si el guard es nulo, significa que el sistema se está deteniendo
                if (!guard) {
                    LOG_INFO("[X3DOutput] Stopping thread (manager stopped).");
                    break;
                }

                // 2. Procesamiento
                std::string filename = "step_" + std::to_string(snapshot.step()) + ".html";

                // Serializamos y escribimos a disco
                X3DSnapshotSerializer serializer;
                serializer.serializeToFile(snapshot, outputDirectory_ / filename);

                // Actualizamos el tracking
                lastProcessedStep = snapshot.step();

                // Logger opcional para debug (puede ser ruidoso si hay muchos pasos)
                // Logger::instance().info("[X3DOutput] Saved " + filename);

                // 3. Finalización automática
                // Al salir del scope (o del try), 'guard' se destruye y llama a signalDone().
            }
            catch (const std::exception& ex) {
                LOG_ERROR("[X3DOutput Error]: {}", ex.what());
                // El bucle continúa, y el 'guard' habrá liberado al Core correctamente.
            }
        }
    }

} // namespace danasim
