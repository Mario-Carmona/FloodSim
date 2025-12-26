
#pragma once

#include <string>

#include "core/ports/InputPort.hpp"

namespace danasim {

    class MQTTInput : public InputPort {
    public:
        explicit MQTTInput(const std::string& broker, const std::string& topic);

        void load(MapGrid& grid) override;

    private:
        std::string broker_;
        std::string topic_;
    };

} // namespace danasim
