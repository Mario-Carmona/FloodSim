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

        virtual void initialize(MapGrid& grid) = 0;

        virtual void step(MapGrid& grid) = 0;

        virtual const ModelParamsInfo& getModelParamsInfo() const = 0;
    };

} // namespace danasim
