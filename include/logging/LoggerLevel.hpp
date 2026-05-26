/**
 * @file LoggerLevel.hpp
 * @brief Defines the logging severity levels for the danasim application.
 * 
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <cstdint>

namespace floodsim {

/**
 * @enum LoggerLevel
 * @brief Defines the severity levels for the logging system.
 */
enum class LoggerLevel : uint8_t {
    kTrace,
    kDebug,
    kInfo,
    kWarn,
    kError,
    kCritical
};

}  // namespace floodsim
