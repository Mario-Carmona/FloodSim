/**
 * @file Types.hpp
 * @brief Global type definitions and aliases for the Danasim simulation engine.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace floodsim {

	/// Type alias used for 2D structural grid coordinate indexing.
	using GridIndexType = std::size_t;

	/// Type alias for flattened 1D array indexing (essential for cache-friendly layer storage).
	using FlatVectorIndexType = std::size_t;

	/// Type alias for tracking discrete simulation time-steps and iteration counts.
	using StepType = std::int64_t;

}  // namespace floodsim
