/**
 * @file InputPort.hpp
 * @brief Interface for data ingestion strategies.
 *
 * Part of the Hexagonal Architecture layer. Defines how the simulation domain
 * receives external datasets (GIS files, APIs, database streams).
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <memory>
#include <string>

#include "app/io/readers/Reader.hpp"

namespace floodsim::app::core::ports {

/**
 * @class InputPort
 * @brief Abstract interface for initializing the simulation domain boundaries.
 *
 * Implementations of this port are responsible for creating readers capable of
 * populating the static and dynamic layers of the MapGrid.
 */
class InputPort {
public:
    virtual ~InputPort() = default;

    /**
     * @brief Factory method to generate a specific Reader implementation.
     * @param name The identification name of the target layer.
     * @param is_static True if the layer holds time-invariant geographical data.
     * @return A std::unique_ptr pointing to the allocated Reader instance.
     */
    virtual std::unique_ptr<io::readers::Reader> GenerateReader(const std::string& name,
                                                                bool is_static) const = 0;

    /**
     * @brief Checks whether a given layer name corresponds to a static dataset.
     * @param name Name of the layer to check.
     * @return true if the layer is static, false otherwise.
     */
    virtual bool IsStaticLayer(const std::string& name) const = 0;

    /**
     * @brief Retrieves the root name of the active input dataset.
     * @return Constant reference to the dataset name string.
     */
    virtual const std::string& GetDatasetName() const = 0;
};

} // namespace floodsim::app::core::ports
