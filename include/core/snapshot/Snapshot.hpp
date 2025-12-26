
#pragma once

#include <cstdint>
#include "core/grid/MapGrid.hpp"

namespace danasim {

    class Snapshot {
    public:
        Snapshot();
        Snapshot(uint64_t step, float time, const MapGrid* grid);

        Snapshot(const Snapshot&) = default;
        Snapshot& operator=(const Snapshot&) = default;
        Snapshot(Snapshot&&) = default;
        Snapshot& operator=(Snapshot&&) = default;

        uint64_t step() const { return step_; }
        float time() const { return time_; }
        const MapGrid* const grid() const { return grid_; }

        bool isValid() const { return grid_ != nullptr; }

    private:
        uint64_t step_;
        float time_;
        const MapGrid* grid_;
    };

} // namespace danasim
