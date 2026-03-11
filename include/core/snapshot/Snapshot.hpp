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

#include "core/grid/MapGrid.hpp"
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
        void set(size_t size);
        void swap(Snapshot& other) noexcept;

        size_t size() const noexcept;

        float* data() noexcept;

        [[nodiscard]] std::chrono::sys_seconds time() const noexcept { return time_; }

        [[nodiscard]] const std::vector<float>& waterDepth() const noexcept { return waterDepth_; }

    private:
        std::chrono::sys_seconds time_;

        std::vector<float> waterDepth_;
    };

} // namespace danasim

// Especialización para que std::swap sea O(1)
namespace std {
    template<> void swap(danasim::Snapshot& a, danasim::Snapshot& b) noexcept;
}
