/**
 * @file Logger.cpp
 * @brief Implementation of the Logger class along with its custom sinks and formatters.
 * 
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "logging/Logger.hpp"

#include <spdlog/async.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <magic_enum/magic_enum.hpp>

#include <atomic>
#include <mutex>
#include <shared_mutex> // C++17
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>

namespace floodsim::logging {

// Use an anonymous namespace for internal linkage
namespace {

    // --- THREAD NAME STORAGE ---
    // Static map: ThreadID -> Name
    std::unordered_map<size_t, std::string> thread_names;
    // Read/write mutex for ultra-fast access during concurrent reads
    std::shared_mutex thread_names_mutex;

    // Atomic variable for dynamic width (starts with a minimum, e.g., 8)
    std::atomic<size_t> max_thread_name_width{ 8 };

    std::shared_ptr<spdlog::logger> global_logger;

}  // namespace

/**
 * @class ThreadNameFlag
 * @brief Custom spdlog flag formatter to inject dynamically aligned thread names.
 */
class ThreadNameFlag : public spdlog::custom_flag_formatter {
public:
    void format(const spdlog::details::log_msg& msg, const std::tm&,
                spdlog::memory_buf_t& dest) override {
        std::string_view name = "Unknown";

        // Read the name (shared lock, very fast)
        {
            std::shared_lock<std::shared_mutex> lock(thread_names_mutex);
            auto it = thread_names.find(msg.thread_id);
            if (it != thread_names.end()) {
                name = it->second;
            }
        }

        // Get the current maximum width atomically
        size_t width = max_thread_name_width.load(std::memory_order_relaxed);

        // Dynamic formatting:
        // "{:<{}}" breaks down as follows:
        //    :   -> Start of format specifications
        //    <   -> Left align
        //    {}  -> The placeholder for width (takes the 'width' argument)
        spdlog::fmt_lib::format_to(std::back_inserter(dest), "{:<{}}", name, width);
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return spdlog::details::make_unique<ThreadNameFlag>();
    }
};

/**
 * @class CallbackSink
 * @brief Custom sink to forward log messages to a generic callback (e.g., for GUIs).
 */
template <typename Mutex>
class CallbackSink : public spdlog::sinks::base_sink<Mutex> {
public:
    explicit CallbackSink(std::function<void(int, const std::string&)> callback)
        : callback_(std::move(callback)) {}

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        this->formatter_->format(msg, formatted);

        if (callback_) {
            // Pass the level (casted to int) and the formatted message
            callback_(static_cast<int>(msg.level), fmt::to_string(formatted));
        }
    }

    void flush_() override {}

private:
    std::function<void(int, const std::string&)> callback_;
};

using CallbackSinkMt = CallbackSink<std::mutex>;

spdlog::level::level_enum Logger::ToSpdLevel(LoggerLevel level) {
    switch (level) {
    case LoggerLevel::kDebug:
        return spdlog::level::debug;
    case LoggerLevel::kInfo:
        return spdlog::level::info;
    case LoggerLevel::kWarn:
        return spdlog::level::warn;
    case LoggerLevel::kError:
        return spdlog::level::err;
    case LoggerLevel::kCritical:
        return spdlog::level::critical;
    default:
        return spdlog::level::info;
    }
}

void Logger::Init(LoggerLevel level, bool async, bool silent,
                  bool save_log_file, const std::filesystem::path& output_path,
                  std::function<void(int, const std::string&)> gui_callback) {
    std::vector<spdlog::sink_ptr> sinks;

    if (silent) {
        sinks.push_back(std::make_shared<spdlog::sinks::null_sink_mt>());
    }
    else {
        if (gui_callback) {
            // GUI MODE: Send logs to the callback
            sinks.push_back(std::make_shared<CallbackSinkMt>(gui_callback));
        }
        else {
            // CLI MODE: Standard console with colors
            sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        }

        if (save_log_file) {
            std::filesystem::path log_file = output_path / "simulation.log";
            sinks.push_back(
                std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file.string(), true));
        }
    }

    if (async && !silent) {
        spdlog::init_thread_pool(8192, 1);
        global_logger = std::make_shared<spdlog::async_logger>(
            "danasim", sinks.begin(), sinks.end(), spdlog::thread_pool(),
            spdlog::async_overflow_policy::block);
    }
    else {
        global_logger = std::make_shared<spdlog::logger>(
            "danasim", sinks.begin(), sinks.end());
    }

    global_logger->set_level(ToSpdLevel(level));

    // Configure formatter
    auto formatter = std::make_unique<spdlog::pattern_formatter>();
    formatter->add_flag<ThreadNameFlag>('*');
    formatter->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%*] [%^%l%$] %v");
    global_logger->set_formatter(std::move(formatter));

    spdlog::set_default_logger(global_logger);

    // Automatically register the main thread
    SetThreadName("Main");
}

void Logger::Shutdown() {
    spdlog::shutdown();
}

std::shared_ptr<spdlog::logger> Logger::Get() {
    return global_logger;
}

void Logger::SetThreadName(const std::string& name) {
    size_t thread_id = spdlog::details::os::thread_id();

    // Save name in the map
    {
        std::unique_lock<std::shared_mutex> lock(thread_names_mutex);
        thread_names[thread_id] = name;
    }

    // Update the maximum width if the new name is longer.
    // We use a CAS (Compare-And-Swap) loop for thread-safety without a mutex.
    size_t len = name.length();
    size_t current_max = max_thread_name_width.load(std::memory_order_relaxed);

    while (len > current_max) {
        // Attempt to update current_max to len.
        // If another thread changed current_max in the meantime, this fails,
        // updates current_max with the real value, and we retry.
        if (max_thread_name_width.compare_exchange_weak(current_max, len)) {
            break;  // Successful update
        }
    }
}

} // namespace floodsim::logging
