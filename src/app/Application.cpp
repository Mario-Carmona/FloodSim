
#include "app/Application.hpp"

#include <exception>
#include <memory>

#include "app/config/ConfigLoader.hpp"
#include "app/factory/InputFactory.hpp"
#include "app/factory/OutputFactory.hpp"
#include "app/factory/StateUpdaterFactory.hpp"
#include "app/logging/Logger.hpp"

#include "core/snapshot/AsyncSnapshotManager.hpp"
#include "core/simulation/SimulationCore.hpp"
#include "utils/Utils.hpp"

namespace danasim {

    Application::Application(const std::string& configPath)
        : configPath_(configPath)
    {
    }

    int Application::run()
    {
        try {
            setCurrentThreadName("Core");

            // -------------------------------------------------
            // 1. Load configuration
            // -------------------------------------------------
            config_ = ConfigLoader::loadFromFile(configPath_);

            Logger::init(
                config_.logging.level,
                config_.logging.async,
                config_.logging.silent,
                config_.logging.file
            );
            Logger::setThreadName("Core");

            LOG_INFO("Application starting");

            // -------------------------------------------------
            // 2. Create input
            // -------------------------------------------------
            LOG_INFO("Creating input module");
            auto input = InputFactory::create(config_.input);

            // -------------------------------------------------
            // 3. Create outputs
            // -------------------------------------------------
            LOG_INFO("Creating output modules");
            outputs_ = OutputFactory::createOutputs(config_.output);

            // -------------------------------------------------
            // 4. Snapshot manager
            // -------------------------------------------------
            snapshotManager_ = std::make_unique<AsyncSnapshotManager>(config_.output.snapshot, outputs_.size());

            // -------------------------------------------------
            // 5. Create state updater
            // -------------------------------------------------
            LOG_INFO("Initializing state updater");
            auto stateUpdater = StateUpdaterFactory::create(config_.stateUpdater);

            // -------------------------------------------------
            // 7. Create core (main thread)
            // -------------------------------------------------
            LOG_INFO("Creating simulation core");

            core_ = std::make_unique<SimulationCore>(
                std::move(stateUpdater),
                input.get(),
                snapshotManager_.get(),
                config_.simulation,
                config_.scenario.name
            );

            // -------------------------------------------------
            // 6. Start output threads
            // -------------------------------------------------
            startOutputThreads();

            // -------------------------------------------------
            // 8. Run simulation
            // -------------------------------------------------
            LOG_INFO("Running simulation");
            core_->run();

            // -------------------------------------------------
            // 9. Shutdown
            // -------------------------------------------------
            LOG_INFO("Application finished");
            snapshotManager_->stop();
            stopOutputThreads();
            Logger::shutdown();
            
            return 0;
        }
        catch (const std::exception& ex) {
            LOG_ERROR("Fatal error: {}", ex.what());
            Logger::shutdown();
            return 1;
        }
    }

    void Application::startOutputThreads()
    {
        for (auto& output : outputs_) {
            outputThreads_.emplace_back(
                [&output, this]() {
                    Logger::setThreadName(output->getThreadName());

                    output->run(*snapshotManager_);
                }
            );

            setThreadName(outputThreads_.back(), output->getThreadName());
        }
    }

    void Application::stopOutputThreads()
    {
        for (auto& t : outputThreads_) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

} // namespace danasim
