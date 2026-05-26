/**
 * @file Application.cpp
 * @brief Implementation of the main Application lifecycle.
 *
 * @copyright Copyright (c) 2026 Danasim
 */

#include "app/Application.hpp"

#include <ctime>
#include <exception>
#include <filesystem>
#include <iostream>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <limits.h>
#include <pthread.h>
#include <unistd.h>
#else
#error "Operating system not supported"
#endif

#include <fmt/chrono.h>
#include <fmt/core.h>

#include "app/adapters/input/FileInput.hpp"
#include "app/adapters/output/OutputFactory.hpp"
#include "app/adapters/state_updater/StateUpdaterFactory.hpp"
#include "app/core/grid/scalars/Scalar.hpp"
#include "logging/Logger.hpp"

namespace floodsim::app {

void SetCurrentThreadName(std::string_view name) {
    // Register name with the Logger first
    logging::Logger::SetThreadName(std::string(name));

#if defined(_WIN32)
    // Windows requires Wide Strings (Unicode)
    if (name.empty()) return;

    // Convert UTF-8/ASCII to WString
    const int size_needed = MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), NULL, 0);
    std::wstring wname(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), &wname[0], size_needed);

    SetThreadDescription(GetCurrentThread(), wname.c_str());
#elif defined(__linux__)
    // Linux / Mac (pthreads)
    // Usually limited to 15 chars + null terminator
    std::string short_name(name.substr(0, 15));
    pthread_setname_np(pthread_self(), short_name.c_str());
#endif
}

std::filesystem::path GetExecutablePath() {
#if defined(_WIN32)
    // Windows implementation
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::filesystem::path(std::string(buffer));
#elif defined(__linux__)
    // Linux implementation
    char buffer[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", buffer, PATH_MAX);
    if (count != -1) {
        return std::filesystem::path(std::string(buffer, count));
    }
    return std::filesystem::path(); // Return empty path on error
#endif
}

Application::Application(const config::Config& config, std::function<void(int, const std::string&)> gui_callback)
    : config_(config) {

    if (config_.scenario.append_start_timestamp) {
        // 1. Get current system time
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);

        // 2. Convert to local time (classic std::tm structure)
        std::tm local_time = *std::localtime(&now_c);

        // 3. Format time string
        std::string timestamp = fmt::format("{:%Y-%m-%d_%H-%M-%S}", local_time);

        config_.scenario.name = config_.scenario.name + "_" + timestamp;
    }
            
    output_path_ = config_.scenario.output_dir / config_.scenario.name;

    if (!std::filesystem::exists(output_path_)) {
        std::filesystem::create_directories(output_path_);
    }

    // Initialize logging subsystem immediately after config load
    logging::Logger::Init(
        config_.logging.level,
        config_.logging.async,
        config_.logging.silent,
        config_.logging.save_log_file,
        output_path_,
        gui_callback
    );
}

Application::~Application()
{
    // Ensure threads stop if the object is destroyed before Run() finishes
    StopOutputThreads();
}

int Application::Run() {
    try {
        // 1. Reset the atomic flag at startup
        stop_requested_.store(false, std::memory_order_relaxed);

        SetCurrentThreadName("Core");

        LOG_INFO("==================================================");
        LOG_INFO("{} v{} - Flood Simulation Engine", FLOODSIM_PROGRAM_NAME, FLOODSIM_PROGRAM_VERSION);
        LOG_INFO("Copyright(c) {} {}", FLOODSIM_COPYRIGHT_YEAR, FLOODSIM_AUTHOR);
        LOG_INFO("==================================================");

        LOG_INFO("Application booting up...");

        // -------------------------------------------------
        // 1. Input Module Initialization
        // -------------------------------------------------
        LOG_INFO("Initializing input factory");

        auto main_input_source = std::make_unique<adapters::input::FileInput>(
            config_.input.file.dataset_folder,
            config_.input.file.dataset_name,
            config_.input.file.static_format,
            config_.input.file.dynamic_format
        );

        std::unordered_map<std::string, core::ports::InputPort*> layers_alternative_input_source;

        // -------------------------------------------------
        // 2. Output Modules Initialization
        // -------------------------------------------------
        LOG_INFO("Initializing output factory");
        outputs_ = adapters::output::OutputFactory::CreateOutputs(config_.output, config_.scenario.name);

        // -------------------------------------------------
        // 3. Snapshot Manager (Async State Capture)
        // -------------------------------------------------
        // Size is based on the number of output ports expecting data
        snapshot_manager_ = std::make_unique<core::snapshot::SnapshotManager>(
            config_.output.snapshot,
            outputs_.size()
        );

        // -------------------------------------------------
        // 4. State Updater (Simulation Logic)
        // -------------------------------------------------
        LOG_INFO("Initializing state updater");
        auto state_updater = adapters::state_updater::StateUpdaterFactory::Create(config_.state_updater);

        // -------------------------------------------------
        // 5. Core Engine Construction
        // -------------------------------------------------
        LOG_INFO("Constructing simulation core");

        std::vector<core::ports::OutputPort*> outputs_ptr;
        outputs_ptr.reserve(outputs_.size());

        for (auto& output : outputs_) {
            outputs_ptr.push_back(output.get());
        }

        core_ = std::make_unique<core::SimulationCore>(
            state_updater.get(),
            main_input_source.get(),
            layers_alternative_input_source,
            config_.input.scalars,
            outputs_ptr,
            snapshot_manager_.get(),
            config_.simulation,
            config_.scenario.name
        );

        // -------------------------------------------------
        // 6. Spin up Output Threads
        // -------------------------------------------------
        StartOutputThreads();

        // -------------------------------------------------
        // 7. Execute Simulation (Blocking)
        // -------------------------------------------------
        LOG_INFO("Starting simulation loop");
        core_->Run(stop_requested_);

        // -------------------------------------------------
        // 8. Graceful Shutdown
        // -------------------------------------------------
        LOG_INFO("Simulation finished. Shutting down services...");

        snapshot_manager_->Stop(); // Flush remaining snapshots
        StopOutputThreads();       // Wait for consumers to finish writing

        logging::Logger::Shutdown();        // Close logs last

        return 0;
    } catch (const std::exception& ex) {
        LOG_ERROR("Fatal application error: {}", ex.what());
        // Attempt clean shutdown of logs even on crash
        logging::Logger::Shutdown();
        return 1;
    }
}

void Application::Stop() {
    // Switch the state flag to true.
    // We use memory_order_relaxed because it is a simple boolean flag 
    // and complex memory barrier synchronization is not required here.
    stop_requested_.store(true, std::memory_order_relaxed);
}

void Application::StartOutputThreads() {
    // Pre-allocate vector to prevent resizing during thread creation
    output_threads_.reserve(outputs_.size());

    for (auto& output : outputs_) {
        // Emplace back directly constructs the jthread
        output_threads_.emplace_back(
            [&output, this](std::stop_token /* st */) {
                // st is a C++20 stop_token (optional use if OutputPort supports it)
                SetCurrentThreadName(output->GetThreadName());

                // Run the output loop (consumer)
                output->Run(*snapshot_manager_, output_path_);
            }
        );
    }
}

void Application::StopOutputThreads() {
    // With std::jthread, threads join automatically on destruction/resize.
    // However, we clear explicitly here to enforce synchronization 
    // before the Logger shuts down.
    output_threads_.clear();
}

} // namespace floodsim::app
