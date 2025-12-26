
#pragma once

#include <memory>

#include "app/config/Config.hpp"
#include "core/ports/InputPort.hpp"

namespace danasim {

    /**
     * @brief Factory responsible for creating input adapters.
     */
    class InputFactory {
    public:
        static std::unique_ptr<InputPort> create(const InputConfig& config);
    };

} // namespace danasim
