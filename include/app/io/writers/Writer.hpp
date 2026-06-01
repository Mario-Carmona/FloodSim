/**
 * @file Writer.hpp
 * @brief Defines the abstract base class for all grid data writers.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "app/core/grid/GridMetadata.hpp"
#include "app/exception/Exception.hpp"

namespace floodsim::app::io::writers {

/**
 * @brief Abstract base class for writing simulation data.
 *
 * Defines the interface for persisting grid data to external storage.
 * Default save implementations throw exceptions to ensure derived classes
 * explicitly support and implement the requested data types.
 */
class Writer {
public:
    Writer() = default;
    virtual ~Writer() = default;

    /**
     * @brief Saves floating-point grid data to the specified path.
     * @throws std::runtime_error if not implemented by the derived class.
     */
    virtual void Save(const std::filesystem::path& /* data_path */,
                      const std::vector<float>& /* data */,
                      const core::grid::GridMetadata& /* metadata */) const {
        throw floodsim::app::exception::FloodSimException("Save for float not implemented in this writer");
    }

    /**
     * @brief Saves 8-bit integer grid data to the specified path.
     * @throws std::runtime_error if not implemented by the derived class.
     */
    virtual void Save(const std::filesystem::path& /* data_path */,
                      const std::vector<int8_t>& /* data */,
                      const core::grid::GridMetadata& /* metadata */) const {
        throw floodsim::app::exception::FloodSimException("Save for int8_t not implemented in this writer");
    }
};

} // namespace floodsim::app::io::writers
