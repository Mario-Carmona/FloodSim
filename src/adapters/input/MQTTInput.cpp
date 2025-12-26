
#include "adapters/input/MQTTInput.hpp"

namespace danasim {

    MQTTInput::MQTTInput(const std::string& broker, const std::string& topic)
        : broker_(broker)
        , topic_(topic)
    {
    }

    void MQTTInput::load(MapGrid& grid)
    {
        
    }

} // namespace danasim
