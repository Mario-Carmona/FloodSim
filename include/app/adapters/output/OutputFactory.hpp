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
#include <string>
#include <vector>

#include "app/config/Config.hpp"
#include "app/core/ports/OutputPort.hpp"

namespace floodsim::app::adapters::output {

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
     * @param scenario_name The name of the current simulation scenario.
     * @return std::vector<std::unique_ptr<OutputPort>> A vector of initialized output adapters.
     * @throws std::invalid_argument If the scenario_name is empty.
     * @throws std::runtime_error If an unknown output configuration type is encountered.
     */
    static std::vector<std::unique_ptr<core::ports::OutputPort>> CreateOutputs(
        const config::OutputConfig& config, const std::string& scenario_name);
};

} // namespace floodsim::app::adapters::output
