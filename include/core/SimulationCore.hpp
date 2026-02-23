
#pragma once

#include <cstddef>
#include <memory>
#include <string>

#include "app/config/Config.hpp"
#include "core/snapshot/SnapshotManager.hpp"
#include "core/snapshot/ChangeList.hpp"
#include "ports/StateUpdaterPort.hpp"
#include "ports/InputPort.hpp"
#include "ports/OutputPort.hpp"
#include "core/grid/MapGrid.hpp"
#include "core/grid/DynamicLayerManager.hpp"

namespace danasim {

    struct ViewRect {
        float minX;
        float maxX;
        float minY;
        float maxY;
    };

    struct ViewGridIndexRect {
        GridIndexType minX;
        GridIndexType maxX;
        GridIndexType minY;
        GridIndexType maxY;
    };

    /**
     * @brief Core simulation engine.
     */
    class SimulationCore {
    public:
        SimulationCore(
            StateUpdaterPort* stateUpdater,
            InputPort* input,
            std::vector<OutputPort*> outputs,
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
        std::vector<OutputPort*> outputs_;
        DynamicLayerManager dynamicLayerManager_;

        // Simulation parameters
        float timeStep_;
        float totalTime_;
        StepType maxSteps_;

        // Identification
        std::string scenarioName_;

        // Simulation state (mutable)
        MapGrid currentGrid_;

        ViewRect view_;

        ViewGridIndexRect viewGridIndex_;

        Snapshot currentSnapshot_;
        Snapshot previousSnapshot_;

        ChangeList changes_;


        struct ThreadResult {
            std::vector<GridIndexType> local_x;
            std::vector<GridIndexType> local_y;
        };

        void process_chunk(int thread_id, GridIndexType start_row, GridIndexType end_row, ThreadResult& result);


        void calculateViewGrid();

        void calculateViewChanges();

        void publishCurrentState(StepType s);
    };

} // namespace danasim
