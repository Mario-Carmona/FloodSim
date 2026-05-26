/**
 * @file ChangeList.hpp
 * @brief Defines the structure for tracking modified cells in a simulation step.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <vector>

#include "misc/Types.hpp"

namespace floodsim {

/**
 * @struct ChangeList
 * @brief Holds a list of spatial indices representing cells that have changed.
 *
 * Used to communicate minimal state updates (deltas) between the physics engine
 * and the output consumers, optimizing network and disk I/O.
 */
struct ChangeList {
    std::vector<GridIndexType> indexes; ///< List of linear grid indices that were updated.
};

} // namespace floodsim
