/**
 * @file OutputFactory.cpp
 * @brief Implementation of the OutputFactory using C++20 visitor pattern.
 */

#include "adapters/output/OutputFactory.hpp"

#include <variant>
#include <stdexcept>
#include <fmt/core.h>

#include "logging/Logger.hpp"
#include "ports/OutputPort.hpp"
#include "app/config/Config.hpp"

// Include concrete implementations
#include "adapters/output/X3dFileOutput.hpp"
#include "adapters/output/MqttOutput.hpp"
#include "adapters/output/ImageOutput.hpp"

namespace danasim {

    namespace {
        // ---------------------------------------------------------------------
        // C++20 Overloaded Visitor Helper
        // ---------------------------------------------------------------------
        template<class... Ts>
        struct overloaded : Ts... { using Ts::operator()...; };

        template<class... Ts>
        overloaded(Ts...) -> overloaded<Ts...>;
    }

    std::vector<std::unique_ptr<OutputPort>>
        OutputFactory::createOutputs(const OutputConfig& config, const std::string& scenarioName)
    {
        std::vector<std::unique_ptr<OutputPort>> outputs;

        // Reserve memory to avoid reallocations
        outputs.reserve(config.outputs.size());

        for (const auto& outCfg : config.outputs) {
            
            // Dispatch creation logic based on the active variant type
            outputs.push_back(std::visit(overloaded{

                // 1. X3D File Output
                [](const OutputConfig::X3dFileOutputConfigEntry&) -> std::unique_ptr<OutputPort> {
                    LOG_DEBUG("Initializing X3D Output");
                    return std::make_unique<X3dFileOutput>();
                },

                // 2. MQTT Output
                [&scenarioName](const OutputConfig::MqttOutputConfigEntry& arg) -> std::unique_ptr<OutputPort> {
                    LOG_DEBUG("Initializing MQTT Output");

                    return std::make_unique<MqttOutput>(
                        arg.protocol + arg.host + ":" + std::to_string(arg.port),
                        scenarioName,
                        arg.payloadFormat
                    );
                },

                // 3. Image Output
                [](const OutputConfig::ImageOutputConfigEntry&) -> std::unique_ptr<OutputPort> {
                    LOG_DEBUG("Initializing Image Output");
                    return std::make_unique<ImageOutput>();
                },

                // 4. Safety catch-all for unhandled types
                [](auto&&) -> std::unique_ptr<OutputPort> {
                    LOG_ERROR("OutputFactory encountered an unknown configuration type.");
                    throw std::runtime_error("OutputFactory: Unknown or unimplemented output type.");
                }

            }, outCfg));
        }

        return outputs;
    }

} // namespace danasim
