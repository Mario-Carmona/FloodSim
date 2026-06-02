/**
 * @file TimeUtils.hpp
 * @brief Inline utility functions for time and duration parsing.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#include "app/exception/Exception.hpp"

namespace floodsim {

/**
 * @brief Parses an ISO 8601 extended timestamp string (YYYY-MM-DDTHH:MM:SS) into UTC seconds.
 *
 * @param timestamp_str The input string to parse.
 * @return std::chrono::sys_seconds A C++20 time point representing UTC system time.
 * @throw floodsim::app::exception::FloodSimException If the string format is invalid or parsing fails.
 */
[[nodiscard]] inline std::chrono::sys_seconds ParseTimestampString(const std::string& timestamp_str) {
    std::istringstream ss{ timestamp_str };
    std::tm time_info = {};

    // %Y-%m-%dT%H:%M:%S is the manual equivalent to the C++20 %FT%T descriptor
    ss >> std::get_time(&time_info, "%Y-%m-%dT%H:%M:%S");

    if (ss.fail()) {
        throw floodsim::app::exception::FloodSimException("Failed to parse timestamp string: " + timestamp_str);
    }

    // Cross-platform conversion from broken-down UTC time (std::tm) to time_t
#if defined(_WIN32) || defined(_WIN64)
    time_t time = _mkgmtime(&time_info);  // Windows (MSVC CRT) UTC variant
#else
    time_t time = timegm(&time_info);     // POSIX (Linux/macOS) UTC variant
#endif

    // Safely cast the system time_point down to the expected seconds resolution
    return std::chrono::time_point_cast<std::chrono::seconds>(
        std::chrono::system_clock::from_time_t(time)
    );
}

/**
 * @brief Parses a duration string formatted as HH:MM:SS into chrono seconds.
 *
 * @param duration_str The input duration string (e.g., "02:30:00").
 * @return std::chrono::seconds Total duration measured in seconds.
 * @throw floodsim::app::exception::FloodSimException If syntax constraints or colon separators are missing.
 */
[[nodiscard]] inline std::chrono::seconds ParseDurationString(const std::string& duration_str) {
    std::istringstream ss{ duration_str };
    int hours = 0;
    int minutes = 0;
    int seconds = 0;
    char colon1 = '\0';
    char colon2 = '\0';

    // Extract time units while capturing individual structural colon separators
    ss >> hours >> colon1 >> minutes >> colon2 >> seconds;

    if (ss.fail() || colon1 != ':' || colon2 != ':') {
        throw floodsim::app::exception::FloodSimException("Failed to parse duration string: " + duration_str);
    }

    return std::chrono::hours(hours) +
        std::chrono::minutes(minutes) +
        std::chrono::seconds(seconds);
}

}  // namespace floodsim
