
#pragma once

#include <cstddef>
#include <memory>
#include "core/grid/MapGrid.hpp"

namespace danasim {

    /**
     * @brief Interface for simulation input providers.
     */
    class InputPort {
    public:
        virtual ~InputPort() = default;

        /**
         * @brief Load and initialize the simulation domain.
         */
        virtual void load(MapGrid& grid) = 0;
    };

} // namespace danasim
