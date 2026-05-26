/**
 * @file StateUpdaterPort.hpp
 * @brief Interface for the core physics/numerical solver simulation engines.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include "misc/Types.hpp"
#include "app/core/grid/MapGrid.hpp"
#include "app/core/snapshot/Snapshot.hpp"

namespace floodsim::app::core::ports {

/**
 * @class StateUpdaterPort
 * @brief Core abstract port used to advance the hydraulic state of the system.
 *
 * Encapsulates the explicit mathematical solver. Implementations may utilize
 * Shallow Water Equations (SWE), Cellular Automata, or Neural Operators.
 */
class StateUpdaterPort {
public:
    /**
     * @brief Constructs a new StateUpdaterPort configuration block.
     * @param enable_rainfall Activates the meteo pluvial forcing data.
     * @param dry_tolerance Minimum water height threshold to consider a cell dry.
     * @param flood_risk_levels Categorization thresholds for hazardous areas.
     */
    StateUpdaterPort(bool enable_rainfall, float dry_tolerance,
                     const std::vector<config::FloodRiskLevel>& flood_risk_levels)
        : enable_rainfall_(enable_rainfall)
        , dry_tolerance_(dry_tolerance)
        , flood_risk_levels_(flood_risk_levels) {}

    virtual ~StateUpdaterPort() = default;

    /**
     * @brief Allocates and links internal physical coefficients onto the grid map.
     * @param grid Reference to the map grid structure to initialize.
     */
    virtual void Initialize(grid::MapGrid& grid) = 0;

    /**
     * @brief Executes a single finite-difference step over the simulation domain.
     * @param grid Mutable reference to the domain layers to process.
     */
    virtual void Step(grid::MapGrid& grid) = 0;

    virtual const grid::ModelParamsInfo& GetModelParamsInfo() const = 0;
    virtual const std::string& GetFluidLayer() const = 0;
    virtual const std::string& GetFluidMovementStateLayer() const = 0;

    bool IsRainfallEnabled() const { return enable_rainfall_; }
    float GetDryTolerance() const { return dry_tolerance_; }

    const std::vector<config::FloodRiskLevel>& GetFloodRiskLevels() const {
        return flood_risk_levels_;
    }

protected:
    /**
     * @brief Optimization enum tracking active kinetic nodes on the sparse grid matrix.
     */
    enum class WaterMovementState : int8_t {
        kStaticState = 0,  ///< Fluid is stationary or dry. No fluxes computed.
        kDynamicState      ///< Kinetic energy presence. Active flux processing.
    };

    static constexpr std::string_view kRainfallLayerName = "rainfall";
    static constexpr std::string_view kFloodRiskLayerName = "flood_risk";

    bool enable_rainfall_;
    float dry_tolerance_;
    std::vector<config::FloodRiskLevel> flood_risk_levels_;
};

} // namespace floodsim::app::core::ports
