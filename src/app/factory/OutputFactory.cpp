/**
 * @file OutputFactory.cpp
 * @brief Implementation of the OutputFactory using C++20 visitor pattern.
 */

#include "app/factory/OutputFactory.hpp"

#include <variant>
#include <stdexcept>
#include <fmt/core.h>

#include "app/logging/Logger.hpp"
#include "core/ports/OutputPort.hpp"
#include "app/config/Config.hpp"

// Include concrete implementations
#include "adapters/output/X3DFileOutput.hpp"
#include "adapters/output/MQTTOutput.hpp"
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
        OutputFactory::createOutputs(const OutputConfig& config)
    {
        std::vector<std::unique_ptr<OutputPort>> outputs;

        // Reserve memory to avoid reallocations
        outputs.reserve(config.outputs.size());

        for (const auto& outCfg : config.outputs) {
            
            // Dispatch creation logic based on the active variant type
            outputs.push_back(std::visit(overloaded{

                // 1. X3D File Output
                [](const OutputConfig::X3DFileOutputConfigEntry& arg) -> std::unique_ptr<OutputPort> {
                    LOG_DEBUG("Initializing X3D Output: {}", arg.filePath.string());
                    return std::make_unique<X3DFileOutput>(arg.filePath);
                },

                // 2. MQTT Output
                [](const OutputConfig::MQTTOutputConfigEntry& arg) -> std::unique_ptr<OutputPort> {
                    LOG_DEBUG("Initializing MQTT Output: {} (Topic: {})", arg.address, arg.topic);

                    return std::make_unique<MQTTOutput>(
                        arg.address,
                        arg.topic,
                        arg.clientId,
                        arg.qos,
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
