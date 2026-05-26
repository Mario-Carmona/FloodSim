/**
 * @file DynamicReader.hpp
 * @brief Defines the interface for readers that handle time-varying data.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <chrono>

#include "app/io/readers/Reader.hpp"

namespace floodsim {

/**
 * @brief Abstract class for readers dealing with dynamic/temporal simulation data.
 */
class DynamicReader : public Reader {
public:
    DynamicReader() = default;
    virtual ~DynamicReader() = default;

    /**
     * @brief Advances or updates the reader state to the specified time.
     * @param current_time The current simulation time point.
     * @return True if the update was successful and new data is available, false otherwise.
     */
    virtual bool Update(std::chrono::system_clock::time_point current_time) = 0;

    /**
     * @brief Retrieves the downgrade factor for spatial or temporal resolution scaling.
     * @return The downgrade factor as an unsigned integer.
     */
    virtual unsigned int GetDowngradeFactor() const = 0;
};

} // namespace floodsim
