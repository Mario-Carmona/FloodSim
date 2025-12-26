
#include "app/factory/StateUpdaterFactory.hpp"

#include <stdexcept>

#include "adapters/state_updater/TensorFlowStateUpdater.hpp"
#include "adapters/state_updater/ExperimentalStateUpdater.hpp"

#include "app/logging/Logger.hpp"

namespace danasim {

    std::unique_ptr<StateUpdaterPort>
        StateUpdaterFactory::create(const StateUpdaterConfig& config)
    {
        return std::visit([](auto&& arg) -> std::unique_ptr<StateUpdaterPort> {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, TensorFlowStateUpdaterConfig>) {
                return std::make_unique<TensorFlowStateUpdater>(arg.modelPath);
            }
            else if constexpr (std::is_same_v<T, ExperimentalStateUpdaterConfig>) {
                return std::make_unique<ExperimentalStateUpdater>();
            }
        }, config);
    }

} // namespace danasim
