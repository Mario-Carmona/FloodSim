
#pragma once

#include "core/ports/StateUpdaterPort.hpp"


namespace danasim {

    class ExperimentalStateUpdater final : public StateUpdaterPort {
    public:
        explicit ExperimentalStateUpdater();
        ~ExperimentalStateUpdater();

        void update(MapGrid& grid, float dt) override;
    };

} // namespace danasim
