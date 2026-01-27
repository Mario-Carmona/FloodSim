/**
 * @file InputFactory.cpp
 * @brief Implementation of the InputFactory using C++20 visitor pattern.
 */

#include "app/factory/InputFactory.hpp"

#include <variant>
#include <stdexcept>
#include <fmt/core.h>

#include "app/logging/Logger.hpp"
#include "core/ports/InputPort.hpp"

// Include concrete implementations
#include "adapters/input/FileInput.hpp"

namespace danasim {

    namespace {
        // ---------------------------------------------------------------------
        // C++20 Visitor Helper (The "Overloaded" Pattern)
        // ---------------------------------------------------------------------
        /**
         * @brief Helper struct to create a visitor from a set of lambdas.
         * Allows writing separate lambdas for each type in a std::variant.
         */
        template<class... Ts>
        struct overloaded : Ts... { using Ts::operator()...; };

        // Explicit deduction guide (C++20 style)
        template<class... Ts>
        overloaded(Ts...) -> overloaded<Ts...>;
    }

    std::unique_ptr<InputPort> InputFactory::create(const InputConfig& config)
    {
        // std::visit dispatches the call to the lambda matching the active variant type.
        return std::visit(overloaded{

            // 1. Handle File Input
            [](const FileInputConfig& cfg) -> std::unique_ptr<InputPort> {
                LOG_DEBUG("Factory creating FileInput source from: {}", cfg.path.string());

                // std::make_unique is optimized for memory allocation
                return std::make_unique<FileInput>(cfg.path);
            },

            // 2. Fallback for unhandled types (Safety Net)
            [](auto&&) -> std::unique_ptr<InputPort> {
                // This branch is hit if a new type is added to InputConfig variant
                // but not handled here yet.
                LOG_ERROR("InputFactory encountered an unhandled configuration type.");
                throw std::runtime_error("InputFactory: Unknown or unimplemented input configuration type.");
            }

        }, config);
    }

} // namespace danasim
