/**
 * @file StateUpdaterFactory.hpp
 * @brief Factory class for creating state updater strategies.
 *
 * This factory isolates the logic for choosing the simulation engine (e.g.,
 * AI-based vs. heuristic-based) based on the provided configuration.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <memory>

#include "app/config/Config.hpp"
#include "app/core/ports/StateUpdaterPort.hpp"

namespace floodsim {

/**
 * @brief Static factory for StateUpdaterPort instances.
 */
class StateUpdaterFactory {
public:
    /**
     * @brief Creates a state updater instance based on the configuration variant.
     *
     * Uses the visitor pattern to dispatch the creation logic associated
     * with the active configuration type (ONNX, Experimental, etc.).
     *
     * @param config The variant holding the specific configuration for the updater.
     * @return std::unique_ptr<StateUpdaterPort> The initialized state updater strategy.
     * @throws std::runtime_error If the specific updater configuration is invalid (e.g., missing model file).
     */
    [[nodiscard]] static std::unique_ptr<StateUpdaterPort> Create(
        const StateUpdaterConfig& config);
};

} // namespace floodsim
