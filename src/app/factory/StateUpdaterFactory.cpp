
/**
 * @file StateUpdaterFactory.cpp
 * @brief Implementation of the StateUpdater factory using C++20 visitor pattern.
 */

#include "app/factory/StateUpdaterFactory.hpp"

#include <variant>
#include <stdexcept>
#include <fmt/core.h>
#include <filesystem>

#include "app/config/Config.hpp"
#include "app/logging/Logger.hpp"
#include "core/ports/StateUpdaterPort.hpp"

// Include concrete implementations
#include "adapters/state_updater/OnnxStateUpdater.hpp"

namespace danasim {

    namespace {
        // ---------------------------------------------------------------------
        // C++20 Visitor Helper
        // ---------------------------------------------------------------------
        template<class... Ts>
        struct overloaded : Ts... { using Ts::operator()...; };

        template<class... Ts>
        overloaded(Ts...) -> overloaded<Ts...>;
    }

    std::unique_ptr<StateUpdaterPort>
        StateUpdaterFactory::create(const StateUpdaterConfig& config)
    {
        return std::visit(overloaded{

            // Onnx Strategy
            [](const OnnxStateUpdaterConfig& cfg) -> std::unique_ptr<StateUpdaterPort> {
                // Validation: Ensure the model file actually exists before passing it to TF
                if (!std::filesystem::exists(cfg.modelPath)) {
                    auto msg = fmt::format("StateUpdaterFactory: Model file not found at '{}'",
                                           cfg.modelPath.string());
                    LOG_ERROR("{}", msg);
                    throw std::runtime_error(msg);
                }

                LOG_DEBUG("Initializing Onnx State Updater using model: {}", cfg.modelPath.string());
                return std::make_unique<OnnxStateUpdater>(cfg.modelPath);
            },

            // Catch-all for unhandled types
            [](auto&&) -> std::unique_ptr<StateUpdaterPort> {
                LOG_ERROR("StateUpdaterFactory: Unknown configuration type encountered.");
                throw std::runtime_error("StateUpdaterFactory: Unimplemented updater type.");
            }

        }, config);
    }

} // namespace danasim
