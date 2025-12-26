
#pragma once

#include <memory>
#include <vector>

#include "core/ports/OutputPort.hpp"
#include "app/config/Config.hpp"

namespace danasim {

    /**
     * @brief Factory for creating output modules from configuration.
     */
    class OutputFactory {
    public:
        static std::vector<std::unique_ptr<OutputPort>>
            createOutputs(const OutputConfig& config);
    };

} // namespace danasim
