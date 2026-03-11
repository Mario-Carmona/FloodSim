/**
 * @file InputFactory.hpp
 * @brief Factory for instantiating input ports based on configuration.
 *
 * This factory isolates the creation logic of input adapters, decoupling
 * the application core from specific implementation classes.
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
    class InputPort;

    /**
     * @class InputFactory
     * @brief Static factory class for creating InputPort instances.
     */
    class InputFactory {
    public:
        /**
         * @brief Creates an InputPort based on the provided configuration variant.
         *
         * Uses std::visit to dispatch the correct constructor based on the
         * active type in the InputConfig variant.
         *
         * @param config The input configuration variant (e.g., FileInputConfig).
         * @return std::unique_ptr<InputPort> The created input adapter.
         * @throw std::runtime_error If the configuration type is not supported.
         */
        static std::unique_ptr<InputPort> create(const InputConfig::InputSourceConfigEntry& config);
    };

} // namespace danasim
