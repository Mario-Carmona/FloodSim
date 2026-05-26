/**
 * @file Snapshot.hpp
 * @brief Immutable value object representing the simulation state at a specific time step.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <vector>

#include "misc/Types.hpp"
#include "app/core/grid/MapGrid.hpp"

namespace floodsim {

/**
 * @class Snapshot
 * @brief A lightweight handle to a simulation frame.
 *
 * This class is designed to be cheap to copy and pass around.
 * It does not own the original MapGrid memory, but caches the relevant
 * simulation layers at a specific point in time for independent consumption.
 */
class Snapshot {
public:
    Snapshot() = default;
    virtual ~Snapshot() = default;

    /**
     * @brief Populates the snapshot data from the current state of the map grid.
     *
     * @param time The timestamp of the current simulation step.
     * @param grid The main map grid containing the simulation layers.
     */
    void Set(std::chrono::sys_seconds time, const MapGrid& grid);

    /**
     * @brief Retrieves the timestamp associated with this snapshot.
     *
     * @return std::chrono::sys_seconds The simulation time.
     */
    [[nodiscard]] std::chrono::sys_seconds Time() const noexcept { return time_; }

    /**
     * @brief Retrieves the cached water depth layer.
     *
     * @return const std::vector<float>& Read-only reference to the water depth data.
     */
    [[nodiscard]] const std::vector<float>& WaterDepth() const noexcept { return water_depth_; }

    /**
     * @brief Retrieves the cached flood risk layer.
     *
     * @return const std::vector<int8_t>& Read-only reference to the flood risk data.
     */
    [[nodiscard]] const std::vector<int8_t>& FloodRisk() const noexcept { return flood_risk_; }

private:
    std::chrono::sys_seconds time_;     ///< Timestamp of the snapshot.

    std::vector<float> water_depth_;    ///< Cached copy of the water depth layer.
    std::vector<int8_t> flood_risk_;    ///< Cached copy of the flood risk layer.
};

} // namespace floodsim
