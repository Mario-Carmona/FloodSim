
#pragma once

#include <source_location>
#include <stdexcept>
#include <string>
#include <fmt/core.h>

namespace floodsim::app::exception {

    class FloodSimException : public std::runtime_error {
    public:
        // Al usar std::source_location::current() como valor por defecto, 
        // el compilador inyecta la ubicación exacta donde se hace el "throw".
        FloodSimException(const std::string& message,
            const std::source_location location = std::source_location::current())
            : std::runtime_error(fmt::format("[{}:{}] {}",
                location.file_name(),
                location.line(),
                message)) {
        }
    };

} // namespace floodsim::app::exception