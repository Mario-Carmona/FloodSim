/**
 * @file MapGrid.hpp
 * @brief Central data structure holding the simulation state.
 *
 * Acts as a container for all spatial data layers and scalars. It manages
 * heterogeneous data types while maintaining contiguous memory vectors for
 * cache efficiency. Also handles georeferencing and coordinate transformations.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <variant>
#include <vector>

#include <fmt/core.h>
#include <magic_enum/magic_enum.hpp>

#include "app/config/Config.hpp"
#include "app/core/grid/layers/DynamicLayer.hpp"
#include "app/core/grid/layers/Layer.hpp"
#include "app/core/grid/layers/StaticLayer.hpp"
#include "app/core/grid/scalars/Scalar.hpp"
#include "app/core/ports/InputPort.hpp"
#include "logging/Logger.hpp"

namespace floodsim::app::core::grid {

/**
 * @enum DataType
 * @brief Defines supported primitive data types for simulation parameters.
 */
enum class DataType {
    kFloat32,
    kInt8
};

/**
 * @struct ModelParam
 * @brief Configuration parameters for a single simulation variable (layer or scalar).
 */
struct ModelParam {
    std::string name;       ///< Identifier of the parameter.
    DataType data_type;     ///< Underlying data type (e.g., float32, int8).
    bool load_required;     ///< Flag indicating if the parameter must be loaded initially.
};

/**
 * @struct ModelParamsInfo
 * @brief Aggregation of all required layers and scalars for the simulation model.
 */
struct ModelParamsInfo {
    std::vector<ModelParam> layers;  ///< List of required spatial layers.
    std::vector<ModelParam> scalars; ///< List of required global scalars.
};

/**
 * @struct GridViewBox
 * @brief Defines a rectangular bounding box using projected grid coordinates.
 */
struct GridViewBox {
    /**
     * @struct Point
     * @brief Represents a 2D coordinate point in projected space.
     */
    struct Point {
        double x;
        double y;
    };

    Point south_west; ///< Bottom-left corner coordinate.
    Point north_east; ///< Top-right corner coordinate.
};

/**
 * @struct UTMZoneInfo
 * @brief Holds parsed Universal Transverse Mercator (UTM) projection data.
 */
struct UTMZoneInfo {
    int zone;      ///< The UTM zone number.
    bool is_north; ///< True if in the northern hemisphere, false otherwise.
};

/**
 * @class MapGrid
 * @brief The primary grid container and orchestrator.
 *
 * Acts as the central hub managing all spatial layers, dynamic updates,
 * scalar values, and coordinate transformations for the simulation environment.
 */
class MapGrid {
public:
    MapGrid() = default;
    ~MapGrid() = default;

    /**
     * @brief Loads and initializes the required simulation layers and scalars.
     *
     * @param params_info Definitions of the layers and scalars needed.
     * @param main_input_source The primary port to read input data from.
     * @param layers_alternative_input_source Map of specific input ports for customized layer loading.
     * @param scalars_config Configuration map for scalar initializations.
     * @param time_step The fundamental time step of the simulation.
     * @param current_time The initial simulation clock time.
     */
    void Load(const ModelParamsInfo& params_info,
        ports::InputPort* main_input_source,
        const std::unordered_map<std::string, ports::InputPort*>& layers_alternative_input_source,
        const std::unordered_map<std::string, std::string>& scalars_config,
        std::chrono::seconds time_step,
        std::chrono::system_clock::time_point current_time);

    /**
     * @brief Polls and updates all dynamic layers based on the current simulation time.
     *
     * @param current_time The current simulation clock time.
     */
    void UpdateDynamicLayers(std::chrono::system_clock::time_point current_time);

    // -------------------------------------------------------------------------
    // Layer Accessors
    // -------------------------------------------------------------------------

    [[nodiscard]] layers::LayerBase* GetLayer(const std::string& name) {
        return layers_.at(name).get();
    }

    [[nodiscard]] const layers::LayerBase* GetLayer(const std::string& name) const {
        return layers_.at(name).get();
    }

    template <typename T>
    [[nodiscard]] layers::Layer<T>* GetLayer(const std::string& name) {
        return dynamic_cast<layers::Layer<T>*>(layers_.at(name).get());
    }

    template <typename T>
    [[nodiscard]] const layers::Layer<T>* GetLayer(const std::string& name) const {
        return dynamic_cast<const layers::Layer<T>*>(layers_.at(name).get());
    }

    // -------------------------------------------------------------------------
    // Scalar Accessors
    // -------------------------------------------------------------------------

    [[nodiscard]] scalars::ScalarBase* GetScalar(const std::string& name) {
        return scalars_.at(name).get();
    }

    [[nodiscard]] const scalars::ScalarBase* GetScalar(const std::string& name) const {
        return scalars_.at(name).get();
    }

    template <typename T>
    [[nodiscard]] scalars::Scalar<T>* GetScalar(const std::string& name) {
        return dynamic_cast<scalars::Scalar<T>*>(scalars_.at(name).get());
    }

    template <typename T>
    [[nodiscard]] const scalars::Scalar<T>* GetScalar(const std::string& name) const {
        return dynamic_cast<const scalars::Scalar<T>*>(scalars_.at(name).get());
    }

    // -------------------------------------------------------------------------
    // Metadata & Georeferencing Getters
    // -------------------------------------------------------------------------

    [[nodiscard]] const GridMetadata& GetMetadata() const { return metadata_; }

    [[nodiscard]] GridIndexType GetRows() const noexcept { return metadata_.height; }
    [[nodiscard]] GridIndexType GetCols() const noexcept { return metadata_.width; }
    [[nodiscard]] float GetCellSize() const noexcept { return metadata_.cell_size; }
    [[nodiscard]] std::string GetCrs() const noexcept { return metadata_.crs; }
    [[nodiscard]] double GetMapOriginX() const noexcept { return metadata_.min_x; }
    [[nodiscard]] double GetMapOriginY() const noexcept { return metadata_.max_y; }
        
    [[nodiscard]] config::ViewBox::Point GetGeoreference() const;

    [[nodiscard]] GridViewBox::Point TransformViewPoint(config::ViewBox::Point source_point, const std::string& target_crs) const;
    [[nodiscard]] config::ViewBox::Point TransformGridViewPoint(GridViewBox::Point source_point, const std::string& target_crs) const;

private:
    std::unordered_map<std::string, std::unique_ptr<layers::LayerBase>> layers_;   ///< Storage for all spatial layers.
    std::unordered_map<std::string, std::unique_ptr<scalars::ScalarBase>> scalars_; ///< Storage for global scalar variables.

    GridMetadata metadata_; ///< Shared spatial metadata for the grid map.

    /**
     * @brief Instantiates and registers a new layer.
     *
     * @tparam T The primitive data type of the layer.
     * @param name Identifier of the layer.
     * @param is_static_layer True to create a StaticLayer, false for a DynamicLayer.
     */
    template <typename T>
    void AddLayer(const std::string& name, bool is_static_layer) {
        if (is_static_layer) {
            layers_[name] = std::make_unique<layers::StaticLayer<T>>(name);
        }
        else {
            layers_[name] = std::make_unique<layers::DynamicLayer<T>>(name);
        }
    }

    /**
     * @brief Instantiates and registers a new scalar.
     *
     * @tparam T The primitive data type of the scalar.
     * @param name Identifier of the scalar.
     */
    template <typename T>
    void AddScalar(const std::string& name) {
        scalars_[name] = std::make_unique<scalars::Scalar<T>>(name);
    }

    /**
     * @brief Standardizes the measurement units for a given layer or scalar.
     *
     * @param name Identifier of the target parameter.
     */
    void NormalizeUnits(const std::string& name);

    /**
     * @brief Parses an EPSG code string to extract UTM zone configuration.
     *
     * @param epsg_string The Coordinate Reference System EPSG string.
     * @return std::optional<UTMZoneInfo> Parsed data if valid, std::nullopt otherwise.
     */
    std::optional<UTMZoneInfo> ParseUTMZoneFromEPSG(const std::string& epsg_string) const;
};

} // namespace floodsim::app::core::grid
