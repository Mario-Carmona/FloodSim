/**
 * @file OutputFactory.hpp
 * @brief Factory for creating simulation output ports.
 *
 * Handles the instantiation of various output adapters (Files, MQTT, Visualization)
 * based on the provided configuration list.
 *
 * @version 1.0
 * @date 2026
 * @copyright Copyright (c) 2026 Danasim
 */

#pragma once

#include <memory>
#include <vector>

#include "app/config/Config.hpp"

namespace danasim {

    // Forward declaration to reduce compile-time dependencies.
    class OutputPort;

    /**
     * @class OutputFactory
     * @brief Static factory for initializing output modules.
     */
    class OutputFactory {
    public:
        /**
         * @brief Creates a list of output ports based on the configuration.
         *
         * Iterates through the output configuration vector and instantiates
         * the corresponding adapter for each entry.
         *
         * @param config The output configuration section containing the list of desired outputs.
         * @return std::vector<std::unique_ptr<OutputPort>> A vector of initialized output adapters.
         * @throw std::runtime_error If an unknown output configuration type is encountered.
         */
        static std::vector<std::unique_ptr<OutputPort>>
            createOutputs(const OutputConfig& config, const std::string& scenarioName);
    };

} // namespace danasim
