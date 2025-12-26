
#include "core/simulation/SimulationCore.hpp"

#include <cmath>

#include "app/logging/Logger.hpp"
#include "core/snapshot/Snapshot.hpp"

namespace danasim {

    SimulationCore::SimulationCore(
        std::unique_ptr<StateUpdaterPort> stateUpdater,
        InputPort* input,
        SnapshotManager* snapshotManager,
        const SimulationConfig& config,
        const std::string& scenarioName
    )
        : stateUpdater_(std::move(stateUpdater))
        , input_(input)
        , snapshotManager_(snapshotManager)
        , timeStep_(config.timeStep)
        , totalTime_(config.totalTime)
        , scenarioName_(scenarioName)
    {
        maxSteps_ = static_cast<std::size_t>(
            std::ceil(totalTime_ / timeStep_)
            );
    }

    void SimulationCore::run()
    {
        LOG_INFO("Simulation started");
        initializeState();

        uint64_t step = 0;
        const auto everyN = snapshotManager_->everyNSteps();

        // Función lambda local para publicar snapshots (DRY - Don't Repeat Yourself)
        auto publishCurrentState = [&](uint64_t s) {
            // 1. Esperar a que los consumidores liberen el buffer anterior
            snapshotManager_->waitForReady();

            // 2. Copiar estado actual al buffer de snapshot
            snapshot_ = active_;

            // 3. Publicar
            snapshotManager_->publish(Snapshot{ s, s * timeStep_, &snapshot_ });

            LOG_INFO("Publish snapshot at step {}", s);
        };

        // Publicar estado inicial (t=0)
        publishCurrentState(step);

        while (step < maxSteps_) {
            // 1. Física
            stateUpdater_->update(active_, timeStep_);
            step++;

            // 2. Output
            if (step % everyN == 0) {
                publishCurrentState(step);
            }
        }

        // Caso borde: Si el último paso no coincidió con everyN, forzamos publicación final
        if (step % everyN != 0) {
            publishCurrentState(step);
        }

        LOG_INFO("Simulation finished");
    }

    void SimulationCore::initializeState()
    {
        input_->load(active_);
    }

} // namespace danasim
