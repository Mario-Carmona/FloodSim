
#pragma once

#include <string>
#include <thread>

#include "app/config/Config.hpp"
#include "core/simulation/SimulationCore.hpp"
#include "core/ports/OutputPort.hpp"

namespace danasim {

    class Application {
    public:
        explicit Application(const std::string& configPath);

        int run();

    private:
        std::string configPath_;
        Config config_;

        std::unique_ptr<SimulationCore> core_;
        std::unique_ptr<SnapshotManager> snapshotManager_;

        std::vector<std::unique_ptr<OutputPort>> outputs_;
        std::vector<std::thread> outputThreads_;

        void startOutputThreads();
        void stopOutputThreads();
    };

} // namespace danasim
