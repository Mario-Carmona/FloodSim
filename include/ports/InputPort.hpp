/**
 * @file InputPort.hpp
 * @brief Interface for data ingestion strategies.
 *
 * @version 1.0
 * @date 2026
 * @copyright Copyright (c) 2026 Danasim
 */

#pragma once

#include <memory>
#include <concepts>

#include "core/grid/MapGrid.hpp"
#include "core/grid/DynamicLayerManager.hpp"

namespace danasim {

    /**
     * @interface InputPort
     * @brief Abstract interface for initializing the simulation domain.
     *
     * Implementations of this class are responsible for populating the static
     * and dynamic layers of the MapGrid from external sources (Files, APIs, Generators).
     */
    class InputPort {
    public:
        virtual ~InputPort() = default;

        /**
         * @brief Loads data into the simulation grid and initializes dynamic layers.
         *
         * @param[out] grid Mutable reference to the main simulation grid.
         * @param[out] dynamicLayerManager Manager for time-variant external data (e.g., HDF5 rain).
         * @param[in] timeStep The simulation time step (dt) in seconds, used for unit conversion during load.
         */
        virtual void load(MapGrid& grid, DynamicLayerManager& dynamicLayerManager, float timeStep) = 0;
    };

} // namespace danasim
