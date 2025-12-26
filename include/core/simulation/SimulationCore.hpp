
#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "app/config/Config.hpp"
#include "core/snapshot/SnapshotManager.hpp"
#include "core/ports/StateUpdaterPort.hpp"
#include "core/ports/InputPort.hpp"
#include "core/grid/MapGrid.hpp"

namespace danasim {

    /**
     * @brief Core simulation engine.
     */
    class SimulationCore {
    public:
        SimulationCore(
            std::unique_ptr<StateUpdaterPort> stateUpdater,
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
        void initializeState();

        // Dependencies
        std::unique_ptr<StateUpdaterPort> stateUpdater_;
        SnapshotManager* snapshotManager_;
        InputPort* input_;

        // Simulation parameters
        float timeStep_;
        float totalTime_;
        std::size_t maxSteps_;

        // Identification
        std::string scenarioName_;

        // Simulation state (mutable)
        MapGrid active_;
        MapGrid snapshot_;
    };

} // namespace danasim
