/**
 * @file Layer.hpp
 * @brief Defines spatial structures, scratchpads, and base layers for grid data management.
 *
 * This file contains the foundational classes for memory management and parallel
 * data updating (using OpenMP) of the simulation's spatial layers.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include <omp.h>

#include "app/io/readers/Reader.hpp"
#include "logging/Logger.hpp"

namespace floodsim::app::core::grid::layers {

/**
 * @struct Tile
 * @brief Represents a processable subdivision of the grid map.
 */
struct Tile {
	int64_t core_start_x; ///< Starting X coordinate of the core area.
	int64_t core_start_y; ///< Starting Y coordinate of the core area.
	int64_t core_width;   ///< Width of the core (may be < 128 at map edges).
	int64_t core_height;  ///< Height of the core (may be < 128 at map edges).
};

/**
 * @struct BoundingBox
 * @brief Defines a rectangular spatial boundary within the grid.
 */
struct BoundingBox {
	int64_t min_x; ///< Minimum X coordinate.
	int64_t min_y; ///< Minimum Y coordinate.
	int64_t max_x; ///< Maximum X coordinate.
	int64_t max_y; ///< Maximum Y coordinate.
};

// -----------------------------------------------------------------------------
// Scratchpad Interfaces & Implementations
// -----------------------------------------------------------------------------

/**
 * @class ScratchpadBase
 * @brief Type-erased base class for temporary memory buffers.
 */
class ScratchpadBase {
public:
	virtual ~ScratchpadBase() = default;

	/**
	 * @brief Resizes the underlying buffer.
	 * @param new_size The new size of the buffer.
	 */
	virtual void Resize(size_t new_size) = 0;

	/**
	 * @brief Gets the current size of the buffer.
	 * @return size_t Current number of elements.
	 */
	[[nodiscard]] virtual size_t GetSize() const noexcept = 0;
};

/**
 * @class Scratchpad
 * @brief Templated temporary memory buffer for layer operations.
 *
 * @tparam T The underlying data type.
 */
template <typename T>
class Scratchpad : public ScratchpadBase {
public:
	Scratchpad() = default;
	virtual ~Scratchpad() = default;

	/**
	 * @brief Gets a reference to the underlying data vector.
	 * @return std::vector<T>& Reference to the data vector.
	 */
	[[nodiscard]] std::vector<T>& GetData() noexcept { return data_; }

	/**
	 * @brief Gets a constant reference to the underlying data vector.
	 * @return const std::vector<T>& Constant reference to the data vector.
	 */
	[[nodiscard]] const std::vector<T>& GetData() const noexcept { return data_; }

	/**
	 * @brief Resizes the underlying data vector.
	 * @param new_size The new size of the vector.
	 */
	void Resize(size_t new_size) override { data_.resize(new_size); }

	/**
	 * @brief Gets the current size of the data vector.
	 * @return size_t Current number of elements.
	 */
	[[nodiscard]] size_t GetSize() const noexcept override { return data_.size(); }

protected:
	/** @brief The underlying data vector. */
	std::vector<T> data_;
};

// -----------------------------------------------------------------------------
// Layer Base & Implementations
// -----------------------------------------------------------------------------

/**
 * @class LayerBase
 * @brief Abstract base class for all spatial grid layers.
 *
 * Provides a common interface to identify layers and determine if their data
 * is dynamically updated during the simulation.
 */
class LayerBase {
public:
	virtual ~LayerBase() = default;

	/**
	 * @brief Retrieves the assigned name of the layer.
	 * @return std::string The layer's identifier.
	 */
	[[nodiscard]] virtual std::string GetName() const = 0;

	/**
	 * @brief Checks whether the layer undergoes dynamic changes.
	 * @return true if the layer is static, false if it is dynamic.
	 */
	[[nodiscard]] virtual bool IsStatic() const noexcept = 0;

	/**
	 * @brief Generates a compatible scratchpad memory buffer for this layer.
	 *
	 * @return std::unique_ptr<ScratchpadBase> An allocated memory buffer matching the layer's specific data type.
	 */
	[[nodiscard]] virtual std::unique_ptr<ScratchpadBase> GenerateScratchpad() const = 0;

	/**
	 * @brief Extracts tile data from the grid into a temporary scratchpad buffer.
	 *
	 * Used typically to prepare sub-grids (tensors) with halo padding for parallel processing or ONNX inference.
	 *
	 * @param scratchpad_base The destination memory buffer.
	 * @param active_tiles List of active tiles mapping the regions to extract.
	 * @param grid_cols Total number of columns in the map grid.
	 * @param grid_rows Total number of rows in the map grid.
	 * @param halo_size Padding size used for neighboring context mapping.
	 * @param tensor_width The physical width of the target tensor tile.
	 * @param tensor_height The physical height of the target tensor tile.
	 */
	virtual void ExtractTilesData(ScratchpadBase* const scratchpad_base,
		const std::vector<Tile>& active_tiles,
		GridIndexType grid_cols,
		GridIndexType grid_rows,
		int64_t halo_size,
		int64_t tensor_width,
		int64_t tensor_height) const = 0;

	/**
	 * @brief Updates internal layer data from a scratchpad buffer using active tiles.
	 *
	 * Usually applies OpenMP to map tile data from the tensor/scratchpad space back into the flat grid array.
	 *
	 * @param scratchpad_base The source memory buffer containing updated tensor data.
	 * @param active_tiles List of tiles mapping the regions being updated.
	 * @param grid_cols Total number of columns in the full map grid.
	 * @param halo_size Padding size to skip (halo is not written back).
	 * @param tensor_width The physical width of the tensor tile.
	 * @param tensor_height The physical height of the tensor tile.
	 */
	virtual void UpdateTilesData(const ScratchpadBase* const scratchpad_base,
		const std::vector<Tile>& active_tiles,
		GridIndexType grid_cols,
		int64_t halo_size,
		int64_t tensor_width,
		int64_t tensor_height) = 0;

	/**
	 * @brief Clears or resets the layer's internal data elements.
	 */
	virtual void Clear() = 0;

	/**
	 * @brief Resizes the underlying contiguous data buffer.
	 *
	 * @param new_size The new capacity size in number of elements.
	 */
	virtual void Resize(size_t new_size) = 0;

	/**
	 * @brief Configures the data reader for initializing or streaming layer data.
	 *
	 * @param main_metadata Metadata describing the global dimensions and properties of the grid.
	 * @param reader A unique pointer transferring ownership of the reader instance to the layer.
	 * @param current_time The initial simulation clock time.
	 */
	virtual void SetReader(const GridMetadata& main_metadata,
		std::unique_ptr<io::readers::Reader> reader,
		std::chrono::system_clock::time_point current_time) = 0;

	/**
	 * @brief Updates the layer's state based on the current simulation time.
	 *
	 * This may trigger reading new data chunks from the configured `Reader` if the layer is dynamic.
	 *
	 * @param current_time The current simulation clock time.
	 */
	virtual void Update(std::chrono::system_clock::time_point current_time) = 0;
};

/**
 * @class Layer
 * @brief Concrete template implementation of a spatial grid layer.
 *
 * Manages a contiguous flat buffer of data representing a single thematic grid
 * layer (e.g., water depth, elevation, or roughness) of type T. Provides high-performance
 * utilities to extract and update grid regions (tiles) using parallel execution.
 *
 * @tparam T The underlying primitive data type stored in the grid cells (e.g., float, int8_t).
 */
template <typename T>
class Layer : public LayerBase {
public:
	/**
	 * @brief Constructs a grid layer with a descriptive name.
	 *
	 * @param name The unique identifier for the layer.
	 */
	explicit Layer(std::string name);

	virtual ~Layer() = default;

	/**
	 * @brief Retrieves the assigned name of the layer.
	 * @return std::string The layer's identifier.
	 */
	[[nodiscard]] std::string GetName() const override { return name_; }

	/**
	 * @brief Generates a compatible scratchpad memory buffer for this specific data type T.
	 *
	 * @return std::unique_ptr<ScratchpadBase> An allocated memory buffer matching the Scratchpad<T> layout.
	 */
	[[nodiscard]] std::unique_ptr<ScratchpadBase> GenerateScratchpad() const override {
		return std::make_unique<Scratchpad<T>>();
	}

	/**
	 * @brief Extracts tile data from the contiguous flat grid into a multi-tile scratchpad tensor buffer.
	 *
	 * Extracts active cells alongside their padding boundary (halo context) to feed external processing kernels.
	 *
	 * @param scratchpad_base The destination memory buffer.
	 * @param active_tiles List of active tiles mapping the regions to extract.
	 * @param grid_cols Total number of columns in the global map grid.
	 * @param grid_rows Total number of rows in the global map grid.
	 * @param halo_size Padding size used for neighboring context mapping.
	 * @param tensor_width The physical width of the target tensor tile.
	 * @param tensor_height The physical height of the target tensor tile.
	 */
	void ExtractTilesData(ScratchpadBase* const scratchpad_base,
		const std::vector<Tile>& active_tiles,
		GridIndexType grid_cols,
		GridIndexType grid_rows,
		int64_t halo_size,
		int64_t tensor_width,
		int64_t tensor_height) const override;

	/**
	 * @brief Updates the flat contiguous grid array from a scratchpad buffer using active tiles.
	 *
	 * Maps processing results back onto the layer, ignoring the outer halo region.
	 *
	 * @param scratchpad_base The source memory buffer containing updated tensor data.
	 * @param active_tiles List of tiles mapping the regions being updated.
	 * @param grid_cols Total number of columns in the full map grid.
	 * @param halo_size Padding size to skip (halo is not written back).
	 * @param tensor_width The physical width of the tensor tile.
	 * @param tensor_height The physical height of the tensor tile.
	 */
	void UpdateTilesData(const ScratchpadBase* const scratchpad_base,
		const std::vector<Tile>& active_tiles,
		GridIndexType grid_cols,
		int64_t halo_size,
		int64_t tensor_width,
		int64_t tensor_height) override;

	/**
	 * @brief Retrieves a mutable reference to the underlying data vector.
	 *
	 * @note This is the only method whose return layout depends explicitly on the template type T.
	 * @return std::vector<T>& Flat array containing the full layer grid cell values.
	 */
	[[nodiscard]] std::vector<T>& GetData() noexcept { return data_; }

	/**
	 * @brief Retrieves an immutable reference to the underlying data vector.
	 *
	 * @note This is the only method whose return layout depends explicitly on the template type T.
	 * @return const std::vector<T>& Flat array containing the full layer grid cell values.
	 */
	[[nodiscard]] const std::vector<T>& GetData() const noexcept { return data_; }

	/**
	 * @brief Clears or resets the internal data elements vector.
	 */
	void Clear() override { data_.clear(); }

	/**
	 * @brief Resizes the underlying contiguous data vector buffer.
	 *
	 * @param new_size The new capacity size in number of elements.
	 */
	void Resize(size_t new_size) override { data_.resize(new_size); }

protected:
	std::string name_;     ///< Assigned identifier of the layer.
	std::vector<T> data_;  ///< Contiguous buffer containing cell data values.
};

// -----------------------------------------------------------------------------
// Template Implementations
// -----------------------------------------------------------------------------

template <typename T>
Layer<T>::Layer(std::string name)
	: name_(std::move(name)) {}

template <typename T>
void Layer<T>::ExtractTilesData(ScratchpadBase* const scratchpad_base,
								const std::vector<Tile>& active_tiles,
								GridIndexType grid_cols,
								GridIndexType grid_rows,
								int64_t halo_size,
								int64_t tensor_width,
								int64_t tensor_height) const {
		
	auto start = std::chrono::high_resolution_clock::now();

	// 1. Robust and Type-Safe assignment of the padding value
	T pad_value;
	if constexpr (std::is_same_v<T, float>) {
		if (name_ == "topo_bathy") {
			pad_value = 9999.0f; // Virtual wall to prevent water from escaping the map edges
		}
		else {
			pad_value = 0.0f;    // e.g., water_depth, rainfall, etc.
		}
	}
	else if constexpr (std::is_same_v<T, int8_t>) {
		pad_value = static_cast<int8_t>(0); // e.g., cell_state, land_cover
	}
	else {
		pad_value = static_cast<T>(0);
	}

	// Safely cast to the concrete Scratchpad type
	auto* scratchpad = dynamic_cast<Scratchpad<T>*>(scratchpad_base);
	if (!scratchpad) {
		LOG_ERROR("ExtractTilesData: Invalid scratchpad type provided for layer '{}'.", name_);
		return;
	}

	T* dest_base_ptr = scratchpad->GetData().data();
	const T* src_base_ptr = data_.data();

	#pragma omp parallel for
	for (int64_t b = 0; b < static_cast<int64_t>(active_tiles.size()); ++b) {
		const auto& tile = active_tiles[b];
		T* batch_dest_ptr = dest_base_ptr + (b * tensor_width * tensor_height);

		// 1. Fill the entire tensor (e.g., 130x130) with the default padding value
		std::fill(batch_dest_ptr, batch_dest_ptr + (tensor_width * tensor_height), pad_value);

		// 2. Calculate real global boundaries (avoiding out-of-bounds map access)
		int64_t start_global_x = tile.core_start_x - halo_size;
		int64_t start_global_y = tile.core_start_y - halo_size;

		int64_t valid_min_x = std::max(int64_t(0), start_global_x);
		int64_t valid_max_x = std::min(static_cast<int64_t>(grid_cols), tile.core_start_x + tile.core_width + halo_size);

		int64_t valid_min_y = std::max(int64_t(0), start_global_y);
		int64_t valid_max_y = std::min(static_cast<int64_t>(grid_rows), tile.core_start_y + tile.core_height + halo_size);

		int64_t valid_width = valid_max_x - valid_min_x;

		for (int64_t gy = valid_min_y; gy < valid_max_y; ++gy) {
			int64_t local_y = gy - start_global_y;
			int64_t local_x = valid_min_x - start_global_x;

			int64_t global_offset = gy * grid_cols + valid_min_x;
			int64_t local_offset = local_y * tensor_width + local_x;

			// Fast memory block transfer for the valid horizontal segment
			std::memcpy(batch_dest_ptr + local_offset, src_base_ptr + global_offset, valid_width * sizeof(T));
		}
	}

	auto end = std::chrono::high_resolution_clock::now();
	LOG_DEBUG("ExtractTilesData ({}) time: {}s", name_, std::chrono::duration<double>(end - start).count());
}

template <typename T>
void Layer<T>::UpdateTilesData(const ScratchpadBase* const scratchpad_base,
							   const std::vector<Tile>& active_tiles,
						       GridIndexType grid_cols,
							   int64_t halo_size,
							   int64_t tensor_width,
							   int64_t tensor_height) {

	auto start = std::chrono::high_resolution_clock::now();

	// Safely cast to the concrete Scratchpad type
	const auto* scratchpad = dynamic_cast<const Scratchpad<T>*>(scratchpad_base);
	if (!scratchpad) {
		LOG_ERROR("UpdateTilesData: Invalid scratchpad type provided for layer '{}'.", name_);
		return;
	}

	T* dest_base_ptr = data_.data();
	const T* src_base_ptr = scratchpad->GetData().data();

	#pragma omp parallel for
	for (int64_t b = 0; b < static_cast<int64_t>(active_tiles.size()); ++b) {
		const auto& tile = active_tiles[b];
		const T* batch_src_ptr = src_base_ptr + (b * tensor_width * tensor_height);

		// WARNING: Iterate over the CORE only. Ignore the halo completely.
		for (int64_t core_y = 0; core_y < tile.core_height; ++core_y) {

			int64_t global_y = tile.core_start_y + core_y;
			int64_t local_y = core_y + halo_size; // Skip the top halo

			int64_t global_offset = (global_y * grid_cols) + tile.core_start_x;
			int64_t local_offset = (local_y * tensor_width) + halo_size; // Skip the left halo

			std::memcpy(dest_base_ptr + global_offset, batch_src_ptr + local_offset, tile.core_width * sizeof(T));
		}
	}

	auto end = std::chrono::high_resolution_clock::now();
	LOG_DEBUG("UpdateTilesData ({}) time: {}s", name_, std::chrono::duration<double>(end - start).count());
}

} // namespace floodsim::app::core::grid::layers