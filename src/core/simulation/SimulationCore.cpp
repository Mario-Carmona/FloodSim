
#include "core/simulation/SimulationCore.hpp"

#include <cmath>

#include "app/logging/Logger.hpp"
#include "core/snapshot/Snapshot.hpp"

namespace danasim {

    SimulationCore::SimulationCore(
        StateUpdaterPort* stateUpdater,
        InputPort* input,
        SnapshotManager* snapshotManager,
        const SimulationConfig& config,
        const std::string& scenarioName
    )
        : stateUpdater_(stateUpdater)
        , input_(input)
        , snapshotManager_(snapshotManager)
        , timeStep_(config.timeStep)
        , totalTime_(config.totalTime)
        , scenarioName_(scenarioName)
        , currentGrid_(config.viewMinX, config.viewMaxX, config.viewMinY, config.viewMaxY)
    {
        maxSteps_ = static_cast<StepType>(std::ceil(totalTime_ / timeStep_));
    }

    void SimulationCore::run()
    {
        LOG_INFO("Simulation started");
        // Initialize State
        input_->load(currentGrid_, streamedLayerManager_, timeStep_);

        stateUpdater_->initialize(currentGrid_, timeStep_, snapshotManager_->everyNSteps());

        currentGrid_.clearLayers();

        StepType step = 0;

        // Función lambda local para publicar snapshots (DRY - Don't Repeat Yourself)
        auto publishCurrentState = [&](StepType s) {
            // 1. Esperar a que los consumidores liberen el buffer anterior
            snapshotManager_->waitForReady();

            // 2. Copiar estado actual al buffer de snapshot
            currentSnapshot_.setStep(s);
            currentSnapshot_.setTime(s * timeStep_);
            
            currentSnapshot_.setCellsDimensions(currentGrid_.rows(), currentGrid_.cols(), &currentGrid_.getLayer<float>(LayerId::Elevation));

            stateUpdater_->render(currentGrid_, currentSnapshot_);

            // 3. Publicar
            snapshotManager_->publish(&currentSnapshot_);

            LOG_INFO("Publish snapshot at step {}", s);
        };

        // Publicar estado inicial (t=0)
        publishCurrentState(step);

        while (step < maxSteps_) {
            // --- PASO 1: CARGAR DATOS EXTERNOS ---
            // Esto lee del HDF5 y escribe en active_.layers_[RainIntensity]
            // streamedLayerManager_.updateAllLayers(active_, step, timeStep_);

            // --- PASO 2: EJECUTAR FÍSICA ---
            // El StateUpdater (Python/TF) leerá RainIntensity como un input más
            stateUpdater_->update();
            
            step += snapshotManager_->everyNSteps();

            // --- PASO 3: OUTPUT ---
            publishCurrentState(step);
        }

        // Caso borde: Si el último paso no coincidió con snapshotManager_->everyNSteps(), forzamos publicación final
        if (step % snapshotManager_->everyNSteps() != 0) {
            publishCurrentState(step);
        }

        LOG_INFO("Simulation finished");
    }

} // namespace danasim
