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

    enum class ScalarId : uint8_t {
        FluidDensity,
        Rainfall,
        LandCover,
        WaterDepth
        // Add new layers here
    };

} // namespace danasim
