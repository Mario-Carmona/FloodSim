
#pragma once

#include <memory>

#include "core/ports/StateUpdaterPort.hpp"
#include "app/config/Config.hpp"

namespace danasim {

    class StateUpdaterFactory {
    public:
        static std::unique_ptr<StateUpdaterPort>
            create(const StateUpdaterConfig& config);
    };

} // namespace danasim
