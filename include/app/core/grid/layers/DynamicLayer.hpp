/**
 * @file DynamicLayer.hpp
 * @brief Defines a grid layer whose data changes dynamically over time.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "app/core/grid/layers/Layer.hpp"
#include "app/io/readers/DynamicReader.hpp"
#include "app/exception/Exception.hpp"

namespace floodsim::app::core::grid::layers {

/**
 * @class DynamicLayer
 * @brief Represents a spatial layer with data that updates periodically during simulation.
 *
 * Dynamic layers (like rainfall or wind) query their underlying readers at each
 * update step. If the temporal data has changed, the grid is refreshed. It supports
 * spatial downgrading (mapping coarse resolution input onto a fine resolution grid).
 *
 * @tparam T The underlying primitive data type of the grid cells.
 */
template <typename T>
class DynamicLayer : public Layer<T> {
public:
    /**
     * @brief Constructs a new Dynamic Layer.
     *
     * @param name The unique identifier for this dynamic layer.
     */
    explicit DynamicLayer(std::string name);

	virtual ~DynamicLayer() = default;

    /**
     * @brief Checks whether the layer undergoes dynamic changes.
     *
     * @return false Since this is a dynamic layer, it updates over time.
     */
    [[nodiscard]] bool IsStatic() const noexcept override { return false; }

    /**
     * @brief Initializes the layer's internal data and sets up the dynamic reader.
     *
     * @param main_metadata Metadata describing the global dimensions and properties.
     * @param reader Unique pointer transferring ownership of the reader instance.
     * @param current_time The initial simulation clock time.
     * @throws std::invalid_argument If the reader is null or cannot be cast to DynamicReader.
     */
    void SetReader(const GridMetadata& main_metadata,
        std::unique_ptr<io::readers::Reader> reader,
        std::chrono::system_clock::time_point current_time) override;

    /**
     * @brief Polls the reader to update the layer's state based on the current simulation time.
     *
     * If the reader indicates a new time frame is available, the layer's data buffer
     * is updated. It handles coordinate mapping if the input has a downgrade factor.
     *
     * @param current_time The current simulation clock time.
     */
    void Update(std::chrono::system_clock::time_point current_time) override;

protected:
    std::unique_ptr<io::readers::DynamicReader> reader_;      ///< Owns the dynamic reader instance.
    std::vector<T> buffer_;                      ///< Coarse temporary buffer for downgraded data.
    std::vector<FlatVectorIndexType> index_map_; ///< Maps fine grid cells to coarse buffer indices.
    std::vector<bool> inside_map_;               ///< Boolean mask indicating if a cell is inside the coverage area.
};

// -----------------------------------------------------------------------------
// Template Implementations
// -----------------------------------------------------------------------------

template <typename T>
DynamicLayer<T>::DynamicLayer(std::string name)
    : Layer<T>(std::move(name)) {}

template <typename T>
void DynamicLayer<T>::SetReader(const GridMetadata& main_metadata,
    std::unique_ptr<io::readers::Reader> reader,
    std::chrono::system_clock::time_point current_time) {
    if (!reader) {
        throw floodsim::app::exception::FloodSimException("DynamicLayer: Provided reader is null.");
    }

    // Safely downcast the Reader to a DynamicReader BEFORE releasing ownership
    auto* dynamic_reader = dynamic_cast<io::readers::DynamicReader*>(reader.get());
    if (!dynamic_reader) {
        throw floodsim::app::exception::FloodSimException("DynamicLayer: Reader must be of type DynamicReader.");
    }

    // Safely transfer ownership now that the type is guaranteed
    reader.release();
    reader_.reset(dynamic_reader);

    // Initialize the main data grid
    this->data_.assign(main_metadata.cell_count, T{});

    // Initialize reader state for the current time
    reader_->Update(current_time);

    unsigned int downgrade_factor = reader_->GetDowngradeFactor();

    if (downgrade_factor > 1) {
        GridMetadata coarse_metadata = reader_->ReadMetadata();

        index_map_.resize(main_metadata.cell_count);
        inside_map_.resize(main_metadata.cell_count, false);

        // Iterate over every cell in the high-resolution Simulation Grid
        for (GridIndexType r = 0; r < static_cast<GridIndexType>(main_metadata.height); ++r) {
            for (GridIndexType c = 0; c < static_cast<GridIndexType>(main_metadata.width); ++c) {
                // Map Sim(r,c) -> Input(hr, hc) using integer division (Downgrading)
                GridIndexType hr = r / downgrade_factor;
                GridIndexType hc = c / downgrade_factor;

                // Boundary Check: Ensure the calculated input coordinate is valid.
                if (hr < static_cast<GridIndexType>(coarse_metadata.height) && hc < static_cast<GridIndexType>(coarse_metadata.width)) {
                    // Store the coarse index mapping
                    FlatVectorIndexType idx = r * main_metadata.width + c;
                    index_map_[idx] = hr * coarse_metadata.width + hc;
                    inside_map_[idx] = true;
                }
            }
        }

        buffer_.resize(coarse_metadata.cell_count, T{});
        reader_->Read(buffer_);

        for (FlatVectorIndexType i = 0; i < inside_map_.size(); ++i) {
            if (inside_map_[i]) {
                // Map the value from the Coarse Buffer -> Fine Cell
                this->data_[i] = buffer_[index_map_[i]];
            }
            else {
                // Cell is outside the coverage area
                this->data_[i] = T{};
            }
        }
    }
    else {
        // Direct 1:1 mapping, read straight into the layer data
        reader_->Read(this->data_);
    }
}

template <typename T>
void DynamicLayer<T>::Update(std::chrono::system_clock::time_point current_time) {
    if (!reader_) return;

    bool updated = reader_->Update(current_time);

    if (updated) {
        if (reader_->GetDowngradeFactor() > 1) {
            reader_->Read(buffer_);

            for (FlatVectorIndexType i = 0; i < inside_map_.size(); ++i) {
                if (inside_map_[i]) {
                    // Map the value from the Coarse Buffer -> Fine Cell
                    this->data_[i] = buffer_[index_map_[i]];
                }
                else {
                    // Cell is outside the coverage area
                    this->data_[i] = T{};
                }
            }
        }
        else {
            reader_->Read(this->data_);
        }
    }
}

} // namespace floodsim::app::core::grid::layers