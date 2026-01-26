
#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "app/config/Config.hpp"
#include "core/snapshot/SnapshotManager.hpp"
#include "core/ports/StateUpdaterPort.hpp"
#include "core/ports/InputPort.hpp"
#include "core/grid/MapGrid.hpp"
#include "core/grid/StreamedLayerManager.hpp"

namespace danasim {

    /**
     * @brief Core simulation engine.
     */
    class SimulationCore {
    public:
        SimulationCore(
            StateUpdaterPort* stateUpdater,
            InputPort* input,
            SnapshotManager* snapshotManager,
            const SimulationConfig& config,
            const std::string& scenarioName
        );

        /**
         * @brief Run the simulation loop.
         */
        void run();

    private:
        // Dependencies
        StateUpdaterPort* stateUpdater_;
        SnapshotManager* snapshotManager_;
        InputPort* input_;
        StreamedLayerManager streamedLayerManager_;

        // Simulation parameters
        float timeStep_;
        float totalTime_;
        StepType maxSteps_;

        // Identification
        std::string scenarioName_;

        // Simulation state (mutable)
        MapGrid currentGrid_;

        Snapshot currentSnapshot_;
    };

} // namespace danasim
