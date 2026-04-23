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
        StateUpdaterPort(bool enableRainfall, float dryTolerance, const std::vector<FloodRiskLevel>& floodRiskLevels)
            : enableRainfall_(enableRainfall), dryTolerance_(dryTolerance), floodRiskLevels_(floodRiskLevels)
        {

        }

        virtual ~StateUpdaterPort() = default;

        virtual void initialize(MapGrid& grid) = 0;

        virtual void step(MapGrid& grid) = 0;

        virtual const ModelParamsInfo& getModelParamsInfo() const = 0;

        virtual const std::string& getFluidLayer() const = 0;
        virtual const std::string& getFluidMovementStateLayer() const = 0;

		virtual bool isRainfallEnabled() const { return enableRainfall_; }

		virtual float getDryTolerance() const { return dryTolerance_; }

		virtual const std::vector<FloodRiskLevel>& getFloodRiskLevels() const { return floodRiskLevels_; }

    protected:
        enum class WaterMovementState : int8_t {
            STATIC_STATE,
            DYNAMIC_STATE
        };

		const std::string RAINFALL_LAYER_NAME = "rainfall";
		const std::string FLOOD_RISK_LAYER_NAME = "flood_risk";

    private:
        bool enableRainfall_;
        float dryTolerance_;
        std::vector<FloodRiskLevel> floodRiskLevels_;
    };

} // namespace danasim
