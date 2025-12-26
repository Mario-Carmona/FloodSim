
#pragma once

#include "core/grid/MapGrid.hpp"

namespace danasim {

    class StateUpdaterPort {
    public:
        virtual ~StateUpdaterPort() = default;

        virtual void update(
            MapGrid& grid,
            float dt
        ) = 0;
    };

} // namespace danasim
