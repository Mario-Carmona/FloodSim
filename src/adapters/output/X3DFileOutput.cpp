
#include "adapters/output/X3DFileOutput.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <fmt/chrono.h>

#include "logging/Logger.hpp"

namespace danasim {

    X3dFileOutput::X3dFileOutput()
    {
        
    }

    void X3dFileOutput::run(SnapshotManager& snapshotManager, const std::filesystem::path& outputPath)
    {
        std::filesystem::path outputDirectory = outputPath / "x3d_files";

        if (!std::filesystem::exists(outputDirectory)) {
            std::filesystem::create_directories(outputDirectory);
        }

        std::chrono::system_clock::time_point lastProcessedStep = std::chrono::system_clock::time_point::min();

        while (true) {
            try {
                // 1. Bloqueo hasta recibir datos (RAII Pattern)
                // Obtenemos el snapshot y el 'guard' que gestiona la señalización
                auto [data, guard] = snapshotManager.waitForSnapshot(lastProcessedStep);
                auto [snapshot, changes] = data;

                // Si el guard es nulo, significa que el sistema se está deteniendo
                if (!guard) {
                    LOG_INFO("Stopping thread (manager stopped).");
                    break;
                }

                // 2. Procesamiento
                std::string filename = fmt::format("step_{:%FT%T}.html", snapshot.time());

                // Serializamos y escribimos a disco
                // X3DSnapshotSerializer serializer;
                // serializer.serializeToFile(snapshot, outputDirectory / filename);

                // Actualizamos el tracking
                lastProcessedStep = snapshot.time();

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

    void X3dFileOutput::setInitConfig(const MapGrid& grid, const std::string& datasetName, std::chrono::sys_seconds /* startTimestamp */) {

    }

} // namespace danasim
