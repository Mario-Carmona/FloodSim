/**
 * @file GridMetadata.hpp
 * @brief Defines the metadata structure for spatial grids.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <string>

namespace floodsim::app::core::grid {

/**
 * @struct GridMetadata
 * @brief Holds spatial dimensions, bounding box, and georeferencing data for a grid.
 *
 * This structure provides all the necessary context to map the continuous
 * physical space (coordinates) into the discrete simulation space (cells/pixels).
 */
struct GridMetadata {
    int width;             ///< Number of columns in the grid.
    int height;            ///< Number of rows in the grid.
    int cell_count;        ///< Total number of cells (width * height).

    float cell_size;       ///< Physical dimension of a single square cell edge.

    double min_x;          ///< Minimum X coordinate (e.g., longitude or easting).
    double max_x;          ///< Maximum X coordinate (e.g., longitude or easting).
    double min_y;          ///< Minimum Y coordinate (e.g., latitude or northing).
    double max_y;          ///< Maximum Y coordinate (e.g., latitude or northing).

    std::string crs;       ///< Coordinate Reference System identifier (e.g., "EPSG:4326").
    std::string units;     ///< Measurement units used in the grid (e.g., "meters", "degrees").
    float unit_dist;       ///< Scale factor or physical distance per defined unit.
};

}