/**
 * @file LayerTypes.hpp
 * @brief distinct types and enumerations for Simulation Layers.
 *
 * Defines the identifiers, data roles, and storage types used across the
 * MapGrid and Input/Output adapters.
 *
 * @version 1.0
 * @date 2026
 * @copyright Copyright (c) 2026 Danasim
 */

#pragma once
#include <cstdint>

namespace danasim {

    /**
     * @enum LayerDataType
     * @brief Underlying storage type for a layer.
     */
    enum class LayerDataType : uint8_t {
        Float32,    ///< Standard continuous data (elevations, depth).
        Int8        ///< Discrete states or boolean flags.
    };

    /**
     * @enum LayerSource
     * @brief Defines the I/O strategy for Principal layers.
     */
    enum class LayerSource : uint8_t {
        Static, ///< Loaded entirely into RAM at startup (e.g., GeoTIFF).
        Dynamic ///< Loaded chunk-by-chunk/frame-by-frame (e.g., HDF5 time series).
    };

} // namespace danasim
