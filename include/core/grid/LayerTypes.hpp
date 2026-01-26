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
     * @enum LayerId
     * @brief Unique identifier for every specific layer in the system.
     */
    enum class LayerId : uint8_t {
        Elevation,
        Rainfall,
        Roughness,
        WaterDepth
        // Add new layers here
    };

    /**
     * @enum LayerCategory
     * @brief semantic categorization of the layer.
     */
    enum class LayerCategory : uint8_t {
        Static,     ///< Never changes during simulation (e.g., Elevation).
        Dynamic     ///< Changes every time step (e.g., WaterDepth).
    };

    /**
     * @enum LayerRole
     * @brief Defines if the layer is loaded from disk or derived internally.
     */
    enum class LayerRole : uint8_t {
        Principal,  ///< Data comes from an external source (File/API).
        Secondary   ///< Data is derived/calculated internally.
    };

    /**
     * @enum LayerSource
     * @brief Defines the I/O strategy for Principal layers.
     */
    enum class LayerSource : uint8_t {
        InMemory,   ///< Loaded entirely into RAM at startup (e.g., GeoTIFF).
        Streamed    ///< Loaded chunk-by-chunk/frame-by-frame (e.g., HDF5 time series).
    };

} // namespace danasim
