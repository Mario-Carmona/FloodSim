
#include "core/snapshot/Snapshot.hpp"

#include "logging/Logger.hpp"

namespace danasim {

    void Snapshot::set(std::chrono::sys_seconds time, const MapGrid& grid) {
        time_ = time;

        const auto& waterDepthGrid = grid.getLayer<float>("water_depth")->getData();
        const auto& floodRiskGrid = grid.getLayer<int8_t>("flood_risk")->getData();

        if (waterDepth_.empty()) {
            waterDepth_.resize(waterDepthGrid.size(), 0.0f);
        }

        if (floodRisk_.empty()) {
            floodRisk_.resize(floodRiskGrid.size(), 0);
        }

        std::memcpy(waterDepth_.data(), waterDepthGrid.data(), waterDepthGrid.size() * sizeof(float));

        std::memcpy(floodRisk_.data(), floodRiskGrid.data(), floodRiskGrid.size() * sizeof(int8_t));
    }

} // namespace danasim
