/**
 * @file Logger.hpp
 * @brief Provides the core Logger class and efficient macros for logging.
 * 
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

 /**
  * @brief Defines the active log level for compilation.
  * * This is optional, but it improves performance if defined before including
  * the spdlog headers, as it completely removes log statements below this
  * level at compile time.
  */
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG

#include <spdlog/spdlog.h>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

#include "logging/LoggerLevel.hpp"

namespace floodsim::logging {

/**
 * @class Logger
 * @brief A wrapper around spdlog providing asynchronous and synchronous logging with custom thread names.
 */
class Logger {
public:
    /**
     * @brief Initializes the global logger with the specified configuration.
     * 
     * @param level The minimum severity level to log.
     * @param async If true, enables asynchronous logging.
     * @param silent If true, suppresses all log output.
     * @param save_log_file If true, writes logs to a file.
     * @param output_path The directory where the log file will be saved.
     * @param gui_callback An optional callback for routing logs to a GUI.
     */
    static void Init(LoggerLevel level, bool async, bool silent,
        bool save_log_file, const std::filesystem::path& output_path,
        std::function<void(int, const std::string&)> gui_callback = nullptr);

    /**
     * @brief Shuts down the logger and flushes all pending messages.
     */
    static void Shutdown();

    /**
     * @brief Retrieves the underlying spdlog logger instance.
     * @return A shared pointer to the raw spdlog logger.
     */
    static std::shared_ptr<spdlog::logger> Get();

    /**
     * @brief Sets a friendly name for the current executing thread.
     * @param name The name to assign to the current thread.
     */
    static void SetThreadName(const std::string& name);

private:
    /**
     * @brief Converts the internal LoggerLevel to the spdlog equivalent.
     * @param level The internal logger level enum.
     * @return The corresponding spdlog level_enum.
     */
    static spdlog::level::level_enum ToSpdLevel(LoggerLevel level);
};

} // namespace floodsim::logging

// --- EFFICIENT ACCESS MACROS ---

/**
 * @brief Logs a debug message using the core logger.
 * * We use macros to automatically capture source location (`__FILE__` and `__LINE__`)
 * and to avoid argument evaluation if the debug log level is disabled.
 * * @param ... Format string and corresponding variadic arguments.
 */
#define LOG_DEBUG(...)    SPDLOG_LOGGER_DEBUG(floodsim::logging::Logger::Get(), __VA_ARGS__)

/**
 * @brief Logs an informational message using the core logger.
 * * We use macros to automatically capture source location (`__FILE__` and `__LINE__`)
 * and to avoid argument evaluation if the info log level is disabled.
 * * @param ... Format string and corresponding variadic arguments.
 */
#define LOG_INFO(...)     SPDLOG_LOGGER_INFO(floodsim::logging::Logger::Get(), __VA_ARGS__)

/**
 * @brief Logs a warning message using the core logger.
 * * We use macros to automatically capture source location (`__FILE__` and `__LINE__`)
 * and to avoid argument evaluation if the warning log level is disabled.
 * * @param ... Format string and corresponding variadic arguments.
 */
#define LOG_WARN(...)     SPDLOG_LOGGER_WARN(floodsim::logging::Logger::Get(), __VA_ARGS__)

/** 
 * @brief Logs an error message using the core logger.
 * * We use macros to automatically capture source location (`__FILE__` and `__LINE__`)
 * and to avoid argument evaluation if the error log level is disabled.
 * * @param ... Format string and corresponding variadic arguments.
 */
#define LOG_ERROR(...)    SPDLOG_LOGGER_ERROR(floodsim::logging::Logger::Get(), __VA_ARGS__)

/**
 * @brief Logs a critical message using the core logger.
 * * We use macros to automatically capture source location (`__FILE__` and `__LINE__`)
 * and to avoid argument evaluation if the critical log level is disabled.
 * * @param ... Format string and corresponding variadic arguments.
 */
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(floodsim::logging::Logger::Get(), __VA_ARGS__)
