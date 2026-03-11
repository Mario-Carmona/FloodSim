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
#include "Types.hpp"
#include "ports/InputPort.hpp"
#include "core/grid/Layer.hpp"

namespace danasim {

    /**
     * @struct LayerDescriptor
     * @brief Metadata defining the properties of a specific layer.
     */
    struct LayerDescriptor {
        std::string name;
        LayerRole role;
    };

    /**
     * @class MapGrid
     * @brief The primary grid container.
     */
    class MapGrid {
    public:
        MapGrid(std::chrono::seconds timeStep);
        ~MapGrid() = default;

        void load(const std::unordered_map<std::string, InputPort*>& layerInputSource, std::chrono::system_clock::time_point currentTime);

        void updateDynamicLayers(std::chrono::system_clock::time_point currentTime);

        LayerBase* getLayer(LayerId id);
        const LayerBase* getLayer(LayerId id) const;

        template <typename T>
        Layer<T>* getLayer(LayerId id) {
            // Llamamos al getter normal y hacemos el casteo internamente
            return dynamic_cast<Layer<T>*>(getLayer(id));
        }

        template <typename T>
        const Layer<T>* getLayer(LayerId id) const {
            return dynamic_cast<const Layer<T>*>(getLayer(id));
        }

        // --- Getters ---

        [[nodiscard]] GridIndexType rows() const noexcept { return layers_[0]->getMetadata().height; }
        [[nodiscard]] GridIndexType cols() const noexcept { return layers_[0]->getMetadata().width; }
        [[nodiscard]] float cellSize() const noexcept { return layers_[0]->getMetadata().cellSize; }
        [[nodiscard]] std::string crs() const noexcept { return layers_[0]->getMetadata().crs; }
        [[nodiscard]] double mapOriginX() const noexcept { return layers_[0]->getMetadata().minX; }
        [[nodiscard]] double mapOriginY() const noexcept { return layers_[0]->getMetadata().maxY; }

    private:
        std::chrono::seconds timeStep_;

        // Storage for all layers defined in the enum
        std::array<std::unique_ptr<LayerBase>, magic_enum::enum_count<LayerId>()> layers_{};


        void initializeDerivedLayer(LayerId id);

        void normalizeUnits(LayerId id);
    };

} // namespace danasim
