/**
 * @file Snapshot.cpp
 * @brief Implementation of the Snapshot class.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/core/snapshot/Snapshot.hpp"

#include <cstring>

#include "logging/Logger.hpp"

namespace floodsim {

void Snapshot::Set(std::chrono::sys_seconds time, const MapGrid& grid) {
    time_ = time;

    const auto& water_depth_grid = grid.GetLayer<float>("water_depth")->GetData();
    const auto& flood_risk_grid = grid.GetLayer<int8_t>("flood_risk")->GetData();

    // Allocate memory if it hasn't been initialized yet
    if (water_depth_.empty()) {
        water_depth_.resize(water_depth_grid.size(), 0.0f);
    }

    if (flood_risk_.empty()) {
        flood_risk_.resize(flood_risk_grid.size(), 0);
    }

    // Perform fast memory copies to cache the grid state
    std::memcpy(water_depth_.data(), water_depth_grid.data(),
        water_depth_grid.size() * sizeof(float));

    std::memcpy(flood_risk_.data(), flood_risk_grid.data(),
        flood_risk_grid.size() * sizeof(int8_t));
}

} // namespace floodsim
