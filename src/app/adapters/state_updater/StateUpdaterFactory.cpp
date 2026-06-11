/**
 * @file StateUpdaterFactory.cpp
 * @brief Implementation of the StateUpdater factory using C++20 visitor pattern.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/adapters/state_updater/StateUpdaterFactory.hpp"

#include <filesystem>
#include <stdexcept>
#include <variant>

#include <fmt/core.h>

#include "app/adapters/state_updater/OnnxStateUpdater.hpp"
#include "app/config/Config.hpp"
#include "logging/Logger.hpp"
#include "app/exception/Exception.hpp"

namespace floodsim::app::adapters::state_updater {

namespace {

/**
 * @brief C++20 Visitor Helper to resolve std::variant matching.
 */
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

}  // namespace

std::unique_ptr<core::ports::StateUpdaterPort> StateUpdaterFactory::Create(
        const config::StateUpdaterConfig& config) {
    return std::visit(
        overloaded{
            // ONNX Strategy
            [&config](const config::OnnxUpdaterConfig& cfg)
                -> std::unique_ptr<core::ports::StateUpdaterPort> {
            // Validation: Ensure the model file actually exists before passing it to ONNX Runtime
            if (!std::filesystem::exists(cfg.model_path)) {
              auto msg = fmt::format(
                  "StateUpdaterFactory: Model file not found at '{}'",
                  cfg.model_path.string());
              LOG_ERROR("{}", msg);
              throw floodsim::app::exception::FloodSimException(msg);
            }

            LOG_DEBUG("Initializing ONNX State Updater using model: {}",
                      cfg.model_path.string());

            return std::make_unique<OnnxStateUpdater>(
                config.enable_rainfall, config.dry_tolerance,
                config.flood_risk_levels, cfg.model_path, cfg.tensor_dim);
          },

        // Catch-all for unhandled configuration types
        [](auto&&) -> std::unique_ptr<core::ports::StateUpdaterPort> {
          LOG_ERROR(
              "StateUpdaterFactory: Unknown configuration type encountered.");
          throw floodsim::app::exception::FloodSimException(
              "StateUpdaterFactory: Unimplemented updater type.");
        }
        },
        config.updater);
}

} // namespace floodsim::app::adapters::state_updater
