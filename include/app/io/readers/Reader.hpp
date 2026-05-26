/**
 * @file Reader.hpp
 * @brief Defines the abstract base class for all grid data readers.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "app/core/grid/GridMetadata.hpp"

namespace floodsim::app::io::readers {

/**
 * @brief Abstract base class for reading simulation data.
 *
 * Defines the interface for extracting grid metadata and numerical data.
 * Default read implementations throw exceptions to ensure derived classes
 * explicitly support the requested data types.
 */
class Reader {
public:
    Reader() = default;
    virtual ~Reader() = default;

    /**
     * @brief Reads and returns the metadata for the grid.
     * @return GridMetadata describing the spatial and structural properties.
     */
    virtual core::grid::GridMetadata ReadMetadata() const = 0;

    /**
     * @brief Reads floating-point data into the provided buffer.
     * @param data The vector to populate with float data.
     * @throws std::runtime_error if not implemented by the derived class.
     */
    virtual void Read(std::vector<float>& /* data */) const {
        throw std::runtime_error("Read for float not implemented in this reader");
    }

    /**
     * @brief Reads 8-bit integer data into the provided buffer.
     * @param data The vector to populate with int8_t data.
     * @throws std::runtime_error if not implemented by the derived class.
     */
    virtual void Read(std::vector<int8_t>& /* data */) const {
        throw std::runtime_error("Read for int8_t not implemented in this reader");
    }
};

} // namespace floodsim::app::io::readers
