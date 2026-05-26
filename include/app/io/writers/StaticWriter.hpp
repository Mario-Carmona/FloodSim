/**
 * @file StaticWriter.hpp
 * @brief Defines the interface for writers that handle static data formats.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include "app/io/writers/Writer.hpp"

namespace floodsim {

/**
 * @brief Abstract class for writing static (non-time-varying) simulation data.
 * * Used for exporting data that does not have a temporal component,
 * such as final simulation states, risk maps, or processed DEMs.
*/
class StaticWriter : public Writer {
public:
    StaticWriter() = default;
    virtual ~StaticWriter() override = default;
};

} // namespace floodsim
