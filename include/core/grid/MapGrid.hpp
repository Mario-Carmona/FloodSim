/**
 * @file MapGrid.hpp
 * @brief Central data structure holding the simulation state.
 *
 * Acts as a container for all spatial data layers. It uses std::variant to
 * support heterogeneous data types (float/int) while maintaining
 * contiguous memory vectors for cache efficiency.
 *
 * @version 1.0
 * @date 2026
 * @copyright Copyright (c) 2026 Danasim
 */

#pragma once

#include <vector>
#include <array>
#include <string>
#include <variant>
#include <cstdint>
#include <stdexcept>
#include <thread>
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <chrono>

#include <magic_enum/magic_enum.hpp>

#include <fmt/core.h>

#include "logging/Logger.hpp"
#include "core/grid/LayerTypes.hpp"
#include "core/grid/ScalarTypes.hpp"
#include "Types.hpp"
#include "ports/InputPort.hpp"
#include "core/grid/Layer.hpp"
#include "core/grid/Scalar.hpp"
#include "app/config/Config.hpp"
#include "core/grid/StaticLayer.hpp"
#include "core/grid/DynamicLayer.hpp"

namespace danasim {

    enum class DataType {
        FLOAT32,
        INT8
    };

    struct ModelParam {
        std::string name;
        DataType dataType;
        bool loadRequired;
    };

    struct ModelParamsInfo {
        std::vector<ModelParam> layers;
        std::vector<ModelParam> scalars;
    };

    /**
     * @class MapGrid
     * @brief The primary grid container.
     */
    class MapGrid {
    public:
        MapGrid();
        ~MapGrid() = default;

        void load(
            const ModelParamsInfo& paramsInfo, InputPort* mainInputSource,
            const std::unordered_map<std::string, InputPort*>& layersAlternativeInputSource,
            const std::unordered_map<std::string, std::string>& scalarsConfig,
            std::chrono::seconds timeStep, std::chrono::system_clock::time_point currentTime
        );

        void updateDynamicLayers(std::chrono::system_clock::time_point currentTime);


        LayerBase* getLayer(const std::string& name) {
            return layers_.at(name).get();
        }

        const LayerBase* getLayer(const std::string& name) const {
            return layers_.at(name).get();
        }


        template <typename T>
        Layer<T>* getLayer(const std::string& name) {
            return dynamic_cast<Layer<T>*>(layers_.at(name).get());
        }

        template <typename T>
        const Layer<T>* getLayer(const std::string& name) const {
            return dynamic_cast<const Layer<T>*>(layers_.at(name).get());
        }


        ScalarBase* getScalar(const std::string& name) {
            return scalars_.at(name).get();
        }

        const ScalarBase* getScalar(const std::string& name) const {
            return scalars_.at(name).get();
        }

        template <typename T>
        Scalar<T>* getScalar(const std::string& name) {
            return dynamic_cast<Scalar<T>*>(scalars_.at(name).get());
        }

        template <typename T>
        const Scalar<T>* getScalar(const std::string& name) const {
            return dynamic_cast<const Scalar<T>*>(scalars_.at(name).get());
        }

        // --- Getters ---

        const GridMetadata& getMetadata() const { return metadata_; }

        [[nodiscard]] GridIndexType rows() const noexcept { return metadata_.height; }
        [[nodiscard]] GridIndexType cols() const noexcept { return metadata_.width; }
        [[nodiscard]] float cellSize() const noexcept { return metadata_.cellSize; }
        [[nodiscard]] std::string crs() const noexcept { return metadata_.crs; }
        [[nodiscard]] double mapOriginX() const noexcept { return metadata_.minX; }
        [[nodiscard]] double mapOriginY() const noexcept { return metadata_.maxY; }

    private:
        // Storage for all layers defined in the enum
        std::unordered_map<std::string, std::unique_ptr<LayerBase>> layers_;

        std::unordered_map<std::string, std::unique_ptr<ScalarBase>> scalars_;

        GridMetadata metadata_;


        template <typename T>
        void addLayer(const std::string& name, bool isStaticLayer) {
            if (isStaticLayer) {
                layers_[name] = std::make_unique<StaticLayer<T>>(name);
            }
            else {
                layers_[name] = std::make_unique<DynamicLayer<T>>(name);
            }
        }

        template <typename T>
        void addScalar(const std::string& name) {
            scalars_[name] = std::make_unique<Scalar<T>>(name);
        }


        void normalizeUnits(const std::string& name);
    };

} // namespace danasim
