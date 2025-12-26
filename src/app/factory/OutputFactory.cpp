
#include "app/factory/OutputFactory.hpp"

#include <stdexcept>

#include "adapters/output/X3DFileOutput.hpp"
#include "adapters/output/MQTTOutput.hpp"

#include "app/factory/SnapshotSerializerFactory.hpp"

#include "app/logging/Logger.hpp"

namespace danasim {

    std::vector<std::unique_ptr<OutputPort>>
        OutputFactory::createOutputs(const OutputConfig& config)
    {
        std::vector<std::unique_ptr<OutputPort>> outputs;

        for (const auto& outCfg : config.outputs) {
            outputs.push_back(
                std::visit([](auto&& arg) -> std::unique_ptr<OutputPort> {
                    using T = std::decay_t<decltype(arg)>;

                    if constexpr (std::is_same_v<T, OutputConfig::X3DFileOutputConfigEntry>) {
                        return std::make_unique<X3DFileOutput>(arg.filePath);
                    }
                    else if constexpr (std::is_same_v<T, OutputConfig::MQTTOutputConfigEntry>) {
                        return std::make_unique<MQTTOutput>(
                            arg.address,
                            arg.topic,
                            arg.clientId,
                            arg.qos,
                            SnapshotSerializerFactory::create(arg.format)
                        );
                    }
                }, outCfg)
            );
        }

        return outputs;
    }

} // namespace danasim
