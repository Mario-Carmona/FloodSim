/**
 * @file FileDynamicFormat.hpp
 * @brief Defines the supported file formats for dynamic simulation data.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <cstdint>

namespace floodsim {

    /**
     * @brief Supported dynamic data file formats.
     */
    enum class FileDynamicFormat : uint8_t {
        kHif  ///< Format identifier for HIF data.
    };

}  // namespace floodsim
