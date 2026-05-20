
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <stdexcept>
#include <optional>
#include <atomic>

#include "app/config/Config.hpp"
#include "app/core/snapshot/SnapshotManager.hpp"
#include "app/core/snapshot/ChangeList.hpp"
#include "app/core/ports/StateUpdaterPort.hpp"
#include "app/core/ports/InputPort.hpp"
#include "app/core/ports/OutputPort.hpp"
#include "app/core/grid/MapGrid.hpp"

namespace danasim {

    struct GridIndexViewBox {
        GridIndexType minCol;
        GridIndexType maxCol;
        GridIndexType minRow;
        GridIndexType maxRow;
    };

    /**
     * @brief Core simulation engine.
     */
    class SimulationCore {
    public:
        SimulationCore(
            StateUpdaterPort* stateUpdater,
            InputPort* mainInputSource,
            const std::unordered_map<std::string, InputPort*>& layersAlternativeInputSource,
            const std::unordered_map<std::string, std::string>& scalarsConfig,
            std::vector<OutputPort*> outputs,
            SnapshotManager* snapshotManager,
            const SimulationConfig& config,
            const std::string& scenarioName
        );

        /**
         * @brief Run the simulation loop.
         */
        void run(const std::atomic<bool>& stopFlag);

    private:
        // Dependencies
        StateUpdaterPort* stateUpdater_;
        SnapshotManager* snapshotManager_;
        InputPort* mainInputSource_;
        std::unordered_map<std::string, InputPort*> layersAlternativeInputSource_;
        std::unordered_map<std::string, std::string> scalarsConfig_;
        std::vector<OutputPort*> outputs_;

        // Simulation parameters
        std::chrono::sys_seconds startTimestamp_;
        std::chrono::seconds simulationDuration_;
        std::chrono::seconds timeStep_;

        // Identification
        std::string scenarioName_;

        // Simulation state (mutable)
        MapGrid currentGrid_;

        ViewBox view_;

        GridIndexViewBox gridIndexView_;

        Snapshot currentSnapshot_;

        ChangeList changes_;


        struct ThreadResult {
            std::vector<GridIndexType> local_indexes;
        };

        void processChunk(GridIndexType start_row, GridIndexType end_row, ThreadResult& result);

        

        void calculateGridView();

        void calculateViewChanges();

        void publishCurrentState(std::chrono::sys_seconds time);
    };

} // namespace danasim
