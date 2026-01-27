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
        // Defaulted comparison operator
        bool operator==(const Snapshot&) const = default;

        /**
         * @brief Constructs an empty/invalid snapshot.
         */
        Snapshot() noexcept
            : step_(0), time_(0.0f)
        {

        }

        Snapshot(const Snapshot&) = default;
        Snapshot& operator=(const Snapshot&) = default;
        Snapshot(Snapshot&&) = default;
        Snapshot& operator=(Snapshot&&) = default;

        /**
         * @return The simulation step number.
         */
        [[nodiscard]] StepType step() const noexcept { return step_; }

        /**
         * @return The accumulated simulation time in seconds.
         */
        [[nodiscard]] float time() const noexcept { return time_; }

        /**
         * @brief Access to topology change tracking vectors.
         * Used to optimize rendering or transmission by only processing changed cells.
         */
        [[nodiscard]] std::vector<GridIndexType>& changedX() { return changed_x_; }
        [[nodiscard]] std::vector<GridIndexType>& changedY() { return changed_y_; }

        [[nodiscard]] const std::vector<GridIndexType>& changedX() const { return changed_x_; }
        [[nodiscard]] const std::vector<GridIndexType>& changedY() const { return changed_y_; }

        [[nodiscard]] std::vector<float>& waterDepth() { return waterDepth_; }
        [[nodiscard]] const std::vector<float>& waterDepth() const { return waterDepth_; }

        [[nodiscard]] std::vector<int8_t>& cellState() { return cellState_; }
        [[nodiscard]] const std::vector<int8_t>& cellState() const { return cellState_; }

        void setStep(StepType value) {
            step_ = value;
        }

        void setTime(float value) {
            time_ = value;
        }

        void setCellsDimensions(GridIndexType rows, GridIndexType cols, std::vector<float>* elevation) {
            rows_ = rows;
            cols_ = cols;
            elevation_ = elevation;
        }

        [[nodiscard]] GridIndexType rows() const noexcept { return rows_; }
		[[nodiscard]] GridIndexType cols() const noexcept { return cols_; }

        [[nodiscard]] const std::vector<float>& elevation() const { return *elevation_; }

    private:
        StepType step_;
        float time_;

        GridIndexType rows_ = 0;
        GridIndexType cols_ = 0;
        std::vector<float>* elevation_;

        // Change tracking
        std::vector<GridIndexType> changed_x_;
        std::vector<GridIndexType> changed_y_;

        std::vector<float> waterDepth_;
        std::vector<int8_t> cellState_;
    };

} // namespace danasim
