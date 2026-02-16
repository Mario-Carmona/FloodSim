
/**
 * @file Application.cpp
 * @brief Implementation of the main Application lifecycle.
 */

#include "app/Application.hpp"

#include <exception>
#include <iostream>
#include <ctime>
#include <fmt/core.h>
#include <fmt/chrono.h>
#include <filesystem>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <pthread.h>
#endif

#include "app/config/ConfigLoader.hpp"
#include "adapters/input/InputFactory.hpp"
#include "adapters/output/OutputFactory.hpp"
#include "adapters/state_updater/StateUpdaterFactory.hpp"
#include "logging/Logger.hpp"

#include "core/snapshot/SnapshotManager.hpp"
#include "core/SimulationCore.hpp"
#include "ports/OutputPort.hpp"

namespace danasim {

    void setCurrentThreadName(std::string_view name) {
        // Register name with the Logger first
        Logger::setThreadName(std::string(name));

    #ifdef _WIN32
        // Windows requires Wide Strings (Unicode)
        // Check for empty to avoid errors
        if (name.empty()) return;

        // Convert UTF-8/ASCII to WString
        const int size_needed = MultiByteToWideChar(CP_UTF8, 0, name.data(), (int)name.size(), NULL, 0);
        std::wstring wName(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, name.data(), (int)name.size(), &wName[0], size_needed);

        SetThreadDescription(GetCurrentThread(), wName.c_str());
    #else
        // Linux / Mac (pthreads)
        // Usually limited to 15 chars + null terminator
        std::string shortName(name.substr(0, 15));
        pthread_setname_np(pthread_self(), shortName.c_str());
    #endif
    }

    Application::Application(const std::filesystem::path& configPath)
        : config_(ConfigLoader::load(configPath))
    {
        // 1. Obtenemos el tiempo actual del sistema
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);

        // 2. Lo convertimos a tiempo local (estructura clásica std::tm)
        // Usamos *std::localtime porque devuelve un puntero
        std::tm localTime = *std::localtime(&now_c);

        // 3. Formateamos
        // fmt soporta std::tm nativamente con la misma sintaxis que tenías
        std::string timestamp = fmt::format("{:%Y-%m-%d_%H-%M-%S}", localTime);

        outputPath_ = config_.scenario.outputDir / (config_.scenario.name + "_" + timestamp);

        if (!std::filesystem::exists(outputPath_)) {
            std::filesystem::create_directories(outputPath_);
        }

        // Initialize logging subsystem immediately after config load
        Logger::init(
            config_.logging.level,
            config_.logging.async,
            config_.logging.silent,
            config_.logging.saveLogFile,
            outputPath_
        );
    }

    Application::~Application()
    {
        // Ensure threads stop if the object is destroyed before run() finishes
        stopOutputThreads();
    }

    int Application::run()
    {
        try {
            setCurrentThreadName("Core");
            LOG_INFO("Application booting up...");

            // -------------------------------------------------
            // 1. Input Module Initialization
            // -------------------------------------------------
            LOG_INFO("Initializing input factory");
            auto input = InputFactory::create(config_.input);

            // -------------------------------------------------
            // 2. Output Modules Initialization
            // -------------------------------------------------
            LOG_INFO("Initializing output factory");
            outputs_ = OutputFactory::createOutputs(config_.output);

            // -------------------------------------------------
            // 3. Snapshot Manager (Async State Capture)
            // -------------------------------------------------
            // Size is based on the number of output ports expecting data
            snapshotManager_ = std::make_unique<SnapshotManager>(
                config_.output.snapshot, 
                outputs_.size()
            );

            // -------------------------------------------------
            // 4. State Updater (Simulation Logic)
            // -------------------------------------------------
            LOG_INFO("Initializing state updater");
            auto stateUpdater = StateUpdaterFactory::create(config_.stateUpdater);

            // -------------------------------------------------
            // 5. Core Engine Construction
            // -------------------------------------------------
            LOG_INFO("Constructing simulation core");

            std::vector<OutputPort*> outputsPtr;

            for (auto& output : outputs_) {
                outputsPtr.push_back(output.get());
            }

            // Note: SimulationCore takes ownership of stateUpdater
            core_ = std::make_unique<SimulationCore>(
                stateUpdater.get(),
                input.get(),
                outputsPtr,
                snapshotManager_.get(),
                config_.simulation,
                config_.scenario.name
            );

            // -------------------------------------------------
            // 6. Spin up Output Threads
            // -------------------------------------------------
            startOutputThreads();

            // -------------------------------------------------
            // 7. Execute Simulation (Blocking)
            // -------------------------------------------------
            LOG_INFO("Starting simulation loop");
            core_->run();

            // -------------------------------------------------
            // 8. Graceful Shutdown
            // -------------------------------------------------
            LOG_INFO("Simulation finished. Shutting down services...");

            snapshotManager_->stop(); // Flush remaining snapshots
            stopOutputThreads();      // Wait for consumers to finish writing

            Logger::shutdown();       // Close logs last
            
            return 0;
        }
        catch (const std::exception& ex) {
            LOG_ERROR("Fatal application error: {}", ex.what());
            // Attempt clean shutdown of logs even on crash
            Logger::shutdown();
            return 1;
        }
    }

    void Application::startOutputThreads()
    {
        // Pre-allocate vector to prevent resizing during thread creation
        outputThreads_.reserve(outputs_.size());

        for (auto& output : outputs_) {
            // Emplace back directly constructs the jthread
            outputThreads_.emplace_back(
                [&output, this](std::stop_token /* st */) {
                    // st is a C++20 stop_token (optional use if OutputPort supports it)

                    setCurrentThreadName(output->getThreadName());

                    // Run the output loop (consumer)
                    output->run(*snapshotManager_, outputPath_);
                }
            );
        }
    }

    void Application::stopOutputThreads()
    {
        // With std::jthread, threads join automatically on destruction/resize.
        // However, we clear explicitly here to enforce synchronization 
        // before the Logger shuts down.
        outputThreads_.clear();
    }

} // namespace danasim
