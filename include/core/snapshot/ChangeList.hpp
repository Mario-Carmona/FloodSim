/**
 * @file Snapshot.hpp
 * @brief Immutable value object representing the simulation state at a specific time step.
 *
 * @version 1.0
 * @date 2026
 * @copyright Copyright (c) 2026 Danasim
 */

#pragma once

#include <vector>

#include "Types.hpp"

namespace danasim {

    struct ChangeList {
        std::vector<GridIndexType> x;
        std::vector<GridIndexType> y;
    };

} // namespace danasim
