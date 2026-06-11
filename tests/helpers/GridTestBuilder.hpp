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
         * @param updater Pointer to the state updater port supplying model parameter
         * info.
         * @param rows Number of rows in the grid layout.
         * @param cols Number of columns in the grid layout.
         */
        static void InitializeGridBase(app::core::grid::MapGrid& grid,
            app::core::ports::StateUpdaterPort* updater,
            GridIndexType rows, GridIndexType cols);

        /**
         * @brief Creates a completely flat lake at rest with uniform water depth.
         *
         * Surrounds the active grid region with perimeter walls elevated 1 meter
         * above the uniform water surface to contain the fluid without generating
         * steep gradients.
         *
         * @param grid Reference to the target MapGrid.
         * @param rows Number of rows in the grid.
         * @param cols Number of columns in the grid.
         * @param water_depth Baseline depth of the water across internal cells.
         * @param updater Pointer to the simulation state updater port.
         */
        static void CreateLakeAtRest(app::core::grid::MapGrid& grid,
            GridIndexType rows, GridIndexType cols,
            float water_depth,
            app::core::ports::StateUpdaterPort* updater);

        /**
         * @brief Creates a lake at rest with a fluctuating, non-uniform bottom
         * topography.
         *
         * The water surface profile remains perfectly flat and horizontal, while the
         * underlying bathymetry varies smoothly using trigonometric functions to test
         * hydrostatic pressure balance on irregular beds.
         *
         * @param grid Reference to the target MapGrid.
         * @param rows Number of rows in the grid.
         * @param cols Number of columns in the grid.
         * @param surface_elevation Absolute water surface level relative to datum.
         * @param updater Pointer to the simulation state updater port.
         */
        static void CreateIrregularLakeAtRest(
            app::core::grid::MapGrid& grid, GridIndexType rows, GridIndexType cols,
            float surface_elevation, app::core::ports::StateUpdaterPort* updater);

        /**
         * @brief Simulates a classic dam break setup bounded within a pool.
         *
         * Concentrates a localized mass of water within a central strip/column.
         *
         * @param grid Reference to the target MapGrid.
         * @param rows Number of rows in the grid.
         * @param cols Number of columns in the grid.
         * @param updater Pointer to the simulation state updater port.
         */
        static void CreateDamBreakInPool(app::core::grid::MapGrid& grid,
            GridIndexType rows, GridIndexType cols,
            app::core::ports::StateUpdaterPort* updater);

        /**
         * @brief Creates a single concentrated cell of water at the center for
         * symmetry testing.
         *
         * Sets up a flat, walled domain containing a small initial fluid drop right
         * at the spatial midpoint. Useful for validating multi-directional scheme
         * symmetry.
         *
         * @param grid Reference to the target MapGrid.
         * @param size Size dimension of the square grid (rows and columns).
         * @param updater Pointer to the simulation state updater port.
         */
        static void CreateDropInCenter(app::core::grid::MapGrid& grid,
            GridIndexType size,
            app::core::ports::StateUpdaterPort* updater);

        /**
         * @brief Generates a completely dry flat terrain layout.
         *
         * Zeroes out all water layers across the grid to verify mass conservation and
         * ensure the solver numerical routines do not generate spurious fluid volumes
         * from nothing.
         *
         * @param grid Reference to the target MapGrid.
         * @param rows Number of rows in the grid.
         * @param cols Number of columns in the grid.
         * @param updater Pointer to the simulation state updater port.
         */
        static void CreateDryTerrain(app::core::grid::MapGrid& grid,
            GridIndexType rows, GridIndexType cols,
            app::core::ports::StateUpdaterPort* updater);

        /**
         * @brief Sets up an adverse sloping plane ramping upwards with a pool at the
         * bottom.
         *
         * Builds a mild 10% slope gradient simulating a natural uphill incline.
         * Initial pooling is restricted to the first quarter of the grid columns to
         * study run-up and gravitational deceleration behaviors.
         *
         * @param grid Reference to the target MapGrid.
         * @param rows Number of rows in the grid.
         * @param cols Number of columns in the grid.
         * @param updater Pointer to the simulation state updater port.
         */
        static void CreateAdverseSlope(app::core::grid::MapGrid& grid,
            GridIndexType rows, GridIndexType cols,
            app::core::ports::StateUpdaterPort* updater);
    };

}  // namespace floodsim::tests
