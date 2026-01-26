/**
 * @file SimulationConstants.hpp
 * @brief Global constant definitions shared across C++ and Python (via parsing).
 *
 * This file defines critical state flags and physical thresholds used by the
 * simulation core.
 *
 * @note **IMPORTANT FOR PYTHON BINDINGS:**
 * The specific syntax `static constexpr int8_t NAME = VALUE;` MUST be preserved
 * exactly as is. The Python config parser relies on regex matching against this
 * pattern to synchronize constants between the Core and the GUI/Controller.
 * Do not change to `enum class` or `auto`.
 *
 * @version 1.0
 * @date 2026
 * @copyright Copyright (c) 2026 Danasim
 */

#pragma once

#include <cstdint>

namespace danasim {

    // =========================================================================
    // CELL TOPOLOGY STATES
    // =========================================================================

    /**
     * @brief Represents a cell with no water or relevant features.
     * Often used as the default initialization value.
     */
    static constexpr int8_t STATE_EMPTY = 0;

    /**
     * @brief Represents a cell that contains water but is currently stable.
     * The flux in/out is below the simulation threshold.
     */
    static constexpr int8_t STATE_STATIC = 1;

    /**
     * @brief Represents a cell with active fluid movement.
     * These cells require full physics calculation (momentum, flux) in the current step.
     */
    static constexpr int8_t STATE_DYNAMIC = 2;
    

    // =========================================================================
    // PHYSICS THRESHOLDS
    // =========================================================================

    /**
     * @brief Minimum water depth (in meters) to consider a cell "wet".
     *
     * Values below this threshold are treated as dry to prevent floating-point
     * underflow errors and to optimize the solver by skipping negligible amounts of water.
     */
    static constexpr float WATER_EPSILON = 1e-3f;

    static constexpr float DANGER_THRESHOLD = 0.3f; // meters

} // namespace danasim
