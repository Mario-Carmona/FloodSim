/**
 * @file Exception.hpp
 * @brief Defines the custom exception class for the FloodSim application.
 */

#pragma once

#include <source_location>
#include <stdexcept>
#include <string>
#include <fmt/core.h>

namespace floodsim::app::exception {

/**
 * @class FloodSimException
 * @brief Base exception class for FloodSim that automatically captures the source location.
 *
 * This class inherits from std::runtime_error. It automatically formats the
 * exception message to include the file name and line number where the
 * exception was thrown, making debugging easier.
 */
class FloodSimException : public std::runtime_error {
public:
    /**
     * @brief Constructs a new FloodSimException.
     *
     * By using std::source_location::current() as the default argument, the
     * compiler automatically injects the exact location where the "throw"
     * statement occurs.
     *
     * @param message The detailed error message explaining the exception.
     * @param location The source location where the exception is thrown. Defaults
     * to the caller's location.
     */
    FloodSimException(const std::string& message,
                      const std::source_location location = std::source_location::current())
        : std::runtime_error(fmt::format("[{}:{}] {}",
                             location.file_name(),
                             location.line(),
                             message)) {}
};

} // namespace floodsim::app::exception