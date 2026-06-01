/**
 * @file StaticLayer.hpp
 * @brief Defines a grid layer whose data remains constant during the simulation.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>

#include "app/core/grid/layers/Layer.hpp"
#include "app/io/readers/StaticReader.hpp"
#include "app/exception/Exception.hpp"

namespace floodsim::app::core::grid::layers {

/**
 * @class StaticLayer
 * @brief Represents a spatial layer with data that does not change over time.
 *
 * Static layers (like topography or initial land cover) are loaded once
 * at initialization and do not require periodic updates.
 *
 * @tparam T The underlying primitive data type of the grid cells.
 */
template <typename T>
class StaticLayer : public Layer<T> {
public:
    /**
     * @brief Constructs a new Static Layer.
     *
     * @param name The unique identifier for this static layer.
     */
    explicit StaticLayer(std::string name);

	virtual ~StaticLayer() override = default;

    /**
     * @brief Checks whether the layer undergoes dynamic changes.
     *
     * @return true Since this is a static layer, it never updates dynamically.
     */
    [[nodiscard]] bool IsStatic() const noexcept override { return true; }

    /**
     * @brief Initializes the layer's internal data using the provided reader.
     *
     * @param main_metadata Metadata describing the global dimensions and properties.
     * @param reader Unique pointer transferring ownership of the reader instance.
     * @param current_time The initial simulation clock time (unused for static layers).
     * @throws std::invalid_argument If the provided reader is null.
     */
    void SetReader(const GridMetadata& main_metadata,
                   std::unique_ptr<io::readers::Reader> reader,
                   std::chrono::system_clock::time_point current_time) override;

    /**
     * @brief Updates the layer's state based on the current simulation time.
     *
     * For StaticLayer, this is a no-op as the data is guaranteed to remain constant.
     *
     * @param current_time The current simulation clock time (unused).
     */
    void Update(std::chrono::system_clock::time_point current_time) override;
};

// -----------------------------------------------------------------------------
// Template Implementations
// -----------------------------------------------------------------------------

template <typename T>
StaticLayer<T>::StaticLayer(std::string name)
    : Layer<T>(std::move(name)) {}

template <typename T>
void StaticLayer<T>::SetReader(const GridMetadata& main_metadata,
                               std::unique_ptr<io::readers::Reader> reader,
                               std::chrono::system_clock::time_point /* current_time */) {

    if (!reader) {
        throw floodsim::app::exception::FloodSimException("StaticLayer: Provided reader is null.");
    }

    // Allocate contiguous memory for the static grid data
    this->data_.assign(main_metadata.cell_count, T{});

    // Read external data into the pre-allocated buffer
    reader->Read(this->data_);
}

template <typename T>
void StaticLayer<T>::Update(std::chrono::system_clock::time_point /* current_time */) {
    // No-op: Static layers do not change over time.
}

} // namespace floodsim::app::core::grid::layers