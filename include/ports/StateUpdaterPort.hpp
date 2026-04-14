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
#include "core/common/SimulationConstants.hpp"

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

    protected:
        enum class FloodRisk : int8_t {
            DRY,
            VERY_SHALLOW,
            LOW_DEPTH,
            MEDIUM_DEPTH,
            HIGH_DEPTH,
            EXTREME_DEPTH
        };

        enum class WaterMovementState : int8_t {
            STATIC_STATE,
            DYNAMIC_STATE
        };

        // 2. Definimos el array utilizando magic_enum para deducir el tamaño exacto.
        // Usamos constexpr para que se evalúe en tiempo de compilación sin coste de ejecución.
        static constexpr std::array<float, magic_enum::enum_count<FloodRisk>()> risk_thresholds = {
            DRY_TOLERANCE,                      // Dry (Máximo 0.0m)
            0.1f,                               // VeryShallow (Máximo 0.1m)
            0.3f,                               // LowDepth (Máximo 0.3m)
            1.0f,                               // MediumDepth (Máximo 1.0m)
            2.0f,                               // HighDepth (Máximo 2.0m)
            std::numeric_limits<float>::max()   // ExtremeDepth (Hasta el infinito)
        };
    };

} // namespace danasim
