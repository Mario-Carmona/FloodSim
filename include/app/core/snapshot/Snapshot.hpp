/**
 * @file Snapshot.hpp
 * @brief Immutable value object representing the simulation state at a specific time step.
 *
 * @version 1.0
 * @date 2026
 * @copyright Copyright (c) 2026 Danasim
 */

#pragma once

#include <cstdint>
#include <chrono>

#include "app/core/grid/MapGrid.hpp"
#include "Types.hpp"

namespace danasim {

    /**
     * @class Snapshot
     * @brief A lightweight handle to a simulation frame.
     *
     * This class is designed to be cheap to copy and pass around.
     * It does not own the MapGrid memory; it only references it.
     */
    class Snapshot {
    public:
        Snapshot() = default;
        virtual ~Snapshot() = default;

        void set(std::chrono::sys_seconds time, const MapGrid& grid);

        [[nodiscard]] std::chrono::sys_seconds time() const noexcept { return time_; }

        [[nodiscard]] const std::vector<float>& waterDepth() const noexcept { return waterDepth_; }
        [[nodiscard]] const std::vector<int8_t>& floodRisk() const noexcept { return floodRisk_; }

    private:
        std::chrono::sys_seconds time_;

        std::vector<float> waterDepth_;
        std::vector<int8_t> floodRisk_;
    };

} // namespace danasim
