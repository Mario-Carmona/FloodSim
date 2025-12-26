
#include "app/factory/InputFactory.hpp"

#include <stdexcept>

#include "adapters/input/FileInput.hpp"
#include "adapters/input/MQTTInput.hpp"

namespace danasim {

    std::unique_ptr<InputPort> InputFactory::create(const InputConfig& config)
    {
        return std::visit([](auto&& arg) -> std::unique_ptr<InputPort> {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, FileInputConfig>) {
                return std::make_unique<FileInput>(arg.path);
            }
            else if constexpr (std::is_same_v<T, MQTTInputConfig>) {
                return std::make_unique<MQTTInput>(
                    arg.broker,
                    arg.topic
                );
            }
        }, config);
    }

} // namespace danasim
