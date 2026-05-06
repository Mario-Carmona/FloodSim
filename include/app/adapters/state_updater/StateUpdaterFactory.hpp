/**
 * @file StateUpdaterFactory.hpp
 * @brief Factory class for creating state updater strategies.
 *
 * This factory isolates the logic for choosing the simulation engine (e.g.,
 * AI-based vs. heuristic-based) based on the provided configuration.
 *
 * @version 1.0
 * @date 2026
 * @copyright Copyright (c) 2026 Danasim
 */

#pragma once

#include <memory>

#include "app/config/Config.hpp"

namespace danasim {

    // Forward declaration to reduce compile-time dependencies.
    class StateUpdaterPort;

    /**
     * @class StateUpdaterFactory
     * @brief Static factory for StateUpdaterPort instances.
     */
    class StateUpdaterFactory {
    public:
        /**
         * @brief Creates a state updater instance based on the configuration variant.
         *
         * Uses the visitor pattern to dispatch the creation logic associated
         * with the active configuration type (TensorFlow, Experimental, etc.).
         *
         * @param config The variant holding the specific configuration for the updater.
         * @return std::unique_ptr<StateUpdaterPort> The initialized state updater strategy.
         * @throw std::runtime_error If the specific updater configuration is invalid (e.g., missing model file).
         */
        [[nodiscard]]
        static std::unique_ptr<StateUpdaterPort>
            create(const StateUpdaterConfig& config);
    };

} // namespace danasim
