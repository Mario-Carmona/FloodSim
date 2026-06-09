/**
 * @file GridTestBuilder.hpp
 * @brief Utility class to generate various terrain and water scenarios for grid testing.
 */

#pragma once

#include "app/core/grid/MapGrid.hpp"
#include "app/core/grid/GridMetadata.hpp"
#include "app/core/ports/StateUpdaterPort.hpp"

namespace floodsim::tests {

    /**
     * @class GridTestBuilder
     * @brief Helper class providing static factory methods to set up specific
     * environment topologies and initial hydraulic states within a MapGrid.
     */
    class GridTestBuilder {
    public:
        /**
         * @brief Initializes the MapGrid structure with base layers and metadata.
         *
         * Allocates and resizes vital layers (such as elevation, water depth, and
         * state flags) to fit the specified dimensions and sets global scalar values.
         *
         * @param grid Reference to the MapGrid instance to be configured.
         * @param updater Pointer to the state updater port supplying model parameter info.
         * @param rows Number of rows in the grid layout.
         * @param cols Number of columns in the grid layout.
         */
        static void InitializeGridBase(
            app::core::grid::MapGrid& grid,
            app::core::ports::StateUpdaterPort* updater,
            GridIndexType rows, GridIndexType cols
        );

        /**
         * @brief Creates a scenario representing a flat lake at rest.
         *
         * Generates a completely flat basin filled with uniform water depth,
         * surrounded by dry perimeter walls to contain the fluid.
         *
         * @param grid Reference to the MapGrid instance.
         * @param rows Number of rows in the grid.
         * @param cols Number of columns in the grid.
         * @param water_depth Initial uniform depth of the water inside the basin.
         * @param updater Pointer to the state updater port.
         */
        static void CreateLakeAtRest(
            app::core::grid::MapGrid& grid,
            GridIndexType rows, GridIndexType cols,
            float water_depth,
            app::core::ports::StateUpdaterPort* updater
        );

        /**
         * @brief Creates a scenario representing a lake at rest with an irregular bed.
         *
         * Generates a sinusoidal bathymetry pattern while maintaining a constant,
         * uniform water surface elevation level. Surrounded by dry perimeter walls.
         *
         * @param grid Reference to the MapGrid instance.
         * @param rows Number of rows in the grid.
         * @param cols Number of columns in the grid.
         * @param surface_elevation Target total water surface elevation height.
         * @param updater Pointer to the state updater port.
         */
        static void CreateIrregularLakeAtRest(
            app::core::grid::MapGrid& grid,
            GridIndexType rows, GridIndexType cols,
            float surface_elevation,
            app::core::ports::StateUpdaterPort* updater
        );

        /**
         * @brief Creates a completely dry flat terrain scenario.
         *
         * Generates flat terrain with absolute zero water depth. Useful for verifying
         * mass conservation and ensuring the network does not generate fluid spontaneously.
         *
         * @param grid Reference to the MapGrid instance.
         * @param rows Number of rows in the grid.
         * @param cols Number of columns in the grid.
         * @param updater Pointer to the state updater port.
         */
        static void CreateDryTerrain(
            app::core::grid::MapGrid& grid,
            GridIndexType rows, GridIndexType cols,
            app::core::ports::StateUpdaterPort* updater
        );

        /**
         * @brief Creates a terrain ramp with an adverse slope and a puddle at the base.
         *
         * Generates an upward ramp ascending along the positive X-axis (columns).
         * A localized puddle of standing water is placed at the lower left section.
         *
         * @param grid Reference to the MapGrid instance.
         * @param rows Number of rows in the grid.
         * @param cols Number of columns in the grid.
         * @param updater Pointer to the state updater port.
         */
        static void CreateAdverseSlope(
            app::core::grid::MapGrid& grid,
            GridIndexType rows, GridIndexType cols,
            app::core::ports::StateUpdaterPort* updater
        );
    };

}  // namespace floodsim::tests
