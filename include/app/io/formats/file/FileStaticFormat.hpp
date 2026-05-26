/**
 * @file FileStaticFormat.hpp
 * @brief Defines the supported file formats for static geographical/simulation data.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <cstdint>

namespace floodsim::app::io::formats::file {

    /**
     * @brief Supported static data file formats.
     */
    enum class FileStaticFormat : uint8_t {
        kIdrisi  ///< Format identifier for IDRISI raster files.
    };

}  // namespace floodsim::app::io::formats::file
