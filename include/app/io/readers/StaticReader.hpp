/**
 * @file StaticReader.hpp
 * @brief Defines the interface for readers that handle immutable, static data.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include "app/io/readers/Reader.hpp"

namespace floodsim {

/**
 * @brief Abstract class for readers dealing with static (non-time-varying) simulation data.
 * * Typical use cases include reading DEMs (Digital Elevation Models) or static maps.
 */
class StaticReader : public Reader {
public:
    StaticReader() = default;
    virtual ~StaticReader() = default;
};

} // namespace floodsim
