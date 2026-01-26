/**
 * @file StateUpdaterPort.hpp
 * @brief Interface for the core physics/AI simulation engine.
 *
 * @version 1.0
 * @date 2026
 * @copyright Copyright (c) 2026 Danasim
 */

#pragma once

#include <cstddef>

#include "core/grid/MapGrid.hpp"
#include "Types.hpp"
#include "core/snapshot/Snapshot.hpp"

namespace danasim {

    /**
     * @interface StateUpdaterPort
     * @brief Abstract interface for advancing the simulation state.
     *
     * This encapsulates the "Solver". Implementations may use physical formulas,
     * cellular automata, or machine learning models (TensorFlow).
     */
    class StateUpdaterPort {
    public:
        virtual ~StateUpdaterPort() = default;

        virtual void initialize(const MapGrid& grid, float dt, StepType steps_batch) = 0;

        /**
         * @brief Advances the simulation by a specified number of steps.
         *
         * @param[in,out] grid The simulation grid containing the current state (WaterDepth, etc.).
         * Values are updated in-place.
         * @param[in] dt The time delta for a single step (in seconds).
         * @param[in] steps_batch The number of iterations to perform in this call.
         * (Batching steps is useful for GPU/TensorFlow overhead reduction).
         */
        virtual void update() = 0;

        virtual void render(const MapGrid& grid, Snapshot& snapshot_) = 0;
    };

} // namespace danasim
