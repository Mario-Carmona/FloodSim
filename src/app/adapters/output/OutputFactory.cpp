/**
 * @file OutputFactory.cpp
 * @brief Implementation of the OutputFactory using C++20 visitor pattern.
 */

#include "app/adapters/output/OutputFactory.hpp"

#include <stdexcept>
#include <variant>

#include <fmt/core.h>

#include "app/adapters/output/CheckpointOutput.hpp"
#include "app/adapters/output/ImageOutput.hpp"
#include "app/adapters/output/MQTTOutput.hpp"
#include "app/config/Config.hpp"
#include "logging/Logger.hpp"

namespace floodsim::app::adapters::output {

namespace {

    /**
     * @brief Helper struct for C++20 std::visit overload pattern.
     * * Allows passing multiple lambdas to std::visit for handling different
     * types in a std::variant cleanly.
     *
     * @tparam Ts Types of the callable objects (lambdas).
     */
    template <class... Ts>
    struct Overloaded : Ts... {
        using Ts::operator()...;
    };

    template <class... Ts>
    Overloaded(Ts...) -> Overloaded<Ts...>;

}  // namespace

std::vector<std::unique_ptr<core::ports::OutputPort>> OutputFactory::CreateOutputs(
        const config::OutputConfig& config, const std::string& scenario_name) {

    // Exception handling: Validate critical input arguments
    if (scenario_name.empty()) {
        LOG_ERROR("Scenario name cannot be empty when creating outputs.");
        throw std::invalid_argument("OutputFactory: scenario_name is empty.");
    }

    std::vector<std::unique_ptr<core::ports::OutputPort>> outputs;

    // Reserve memory to avoid reallocations
    outputs.reserve(config.outputs.size());

    for (const auto& out_cfg : config.outputs) {
            
        // Dispatch creation logic based on the active variant type
        outputs.push_back(std::visit(
            Overloaded{
                // 1. Checkpoint Output
                [](const config::OutputConfig::CheckpointOutputConfigEntry& arg)
                    -> std::unique_ptr<core::ports::OutputPort> {
                    LOG_DEBUG("Initializing Checkpoint Output");
                    return std::make_unique<CheckpointOutput>(arg.static_format);
                },

            // 2. MQTT Output
            [&scenario_name](const config::OutputConfig::MqttOutputConfigEntry& arg)
                -> std::unique_ptr<core::ports::OutputPort> {
                LOG_DEBUG("Initializing MQTT Output");
                return std::make_unique<MqttOutput>(
                    arg.protocol + arg.host + ":" + std::to_string(arg.port),
                    scenario_name, arg.payload_format, arg.send_initial_state);
            },

            // 3. Image Output
            [](const config::OutputConfig::ImageOutputConfigEntry&)
                -> std::unique_ptr<core::ports::OutputPort> {
                LOG_DEBUG("Initializing Image Output");
                return std::make_unique<ImageOutput>();
            },

            // 4. Safety catch-all for unhandled types
            [](auto&&) -> std::unique_ptr<core::ports::OutputPort> {
                LOG_ERROR("OutputFactory encountered an unknown configuration type.");
                throw std::runtime_error(
                    "OutputFactory: Unknown or unimplemented output type.");
            } },
            out_cfg));
    }

    return outputs;
}

} // namespace floodsim::app::adapters::output
