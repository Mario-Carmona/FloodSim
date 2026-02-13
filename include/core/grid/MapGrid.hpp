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

#include <magic_enum/magic_enum.hpp>

#include <fmt/core.h>

#include "app/logging/Logger.hpp"
#include "core/grid/LayerTypes.hpp"
#include "Types.hpp"

namespace danasim {

    /**
     * @struct LayerDescriptor
     * @brief Metadata defining the properties of a specific layer.
     */
    struct LayerDescriptor {
        std::string name;
        LayerCategory category;
        LayerRole role;
        LayerDataType dataType;
        LayerSource source;
    };

    /**
     * @typedef LayerDataVariant
     * @brief Polymorphic storage for layer data.
     * Uses monostate for default initialization before data is loaded.
     */
    using LayerDataVariant = std::variant<std::monostate, std::vector<float>, std::vector<int8_t>>;

    /**
     * @class MapGrid
     * @brief The primary grid container.
     */
    class MapGrid {
    public:
        MapGrid() = default;
        ~MapGrid() = default;

        /**
         * @brief Initializes the grid dimensions and allocates memory for all layers.
         * * This method acts as the primary setup for the simulation environment.
         * It calculates the total cell count, resets change tracking vectors, and
         * allocates the underlying memory buffers for every layer defined in `LayerId`.
         * * @note This wipes any existing data in the grid.
         * * @param rows The height of the grid (number of cells along the Y-axis).
         * @param cols The width of the grid (number of cells along the X-axis).
         * @param cellSize The spatial resolution of each cell (e.g., in meters).
         * * @throw std::invalid_argument If rows or cols are zero.
         * @throw std::bad_alloc If there is insufficient memory to allocate the grid.
         */
        void init(GridIndexType rows, GridIndexType cols, float cellSize, float mapOriginX, float mapOriginY);

        /**
         * @brief Access the raw data vector for a specific layer.
         *
         * @tparam T The expected data type (float or int8_t).
         * @param id The Layer ID.
         * @return std::vector<T>& Reference to the underlying data.
         * @throw std::runtime_error If the requested type T does not match the layer's storage.
         */
        template <typename T>
        [[nodiscard]] std::vector<T>& getLayer(LayerId id) {
            size_t index = static_cast<size_t>(id);
            if (std::holds_alternative<std::vector<T>>(layers_[index])) {
                return std::get<std::vector<T>>(layers_[index]);
            }

            auto msg = fmt::format("MapGrid: Type mismatch for layer '{}'", magic_enum::enum_name(id));
            LOG_ERROR("{}", msg);
            throw std::runtime_error(msg);
        }

        template <typename T>
        [[nodiscard]] const std::vector<T>& getLayer(LayerId id) const {
            size_t index = static_cast<size_t>(id);
            if (std::holds_alternative<std::vector<T>>(layers_[index])) {
                return std::get<std::vector<T>>(layers_[index]);
            }
            
            auto msg = fmt::format("MapGrid: Type mismatch for layer '{}'", magic_enum::enum_name(id));
            LOG_ERROR("{}", msg);
            throw std::runtime_error(msg);
        }

        /**
         * @brief Obtiene el puntero crudo a los datos de la capa para interoperabilidad (Zero-Copy).
         * Retorna nullptr si la capa no es del tipo T o está vacía.
         */
        template <typename T>
        T* getLayerDataRaw(LayerId id) {
            size_t index = static_cast<size_t>(id);
            auto& variant = layers_[index];

            if (std::holds_alternative<std::vector<T>>(variant)) {
                std::vector<T>& vec = std::get<std::vector<T>>(variant);
                return vec.empty() ? nullptr : vec.data();
            }
            return nullptr;
        }

        // Versión const para inputs de solo lectura
        template <typename T>
        const T* getLayerDataRaw(LayerId id) const {
            size_t index = static_cast<size_t>(id);
            auto& variant = layers_[index];

            if (std::holds_alternative<std::vector<T>>(variant)) {
                const std::vector<T>& vec = std::get<std::vector<T>>(variant);
                return vec.empty() ? nullptr : vec.data();
            }
            return nullptr;
        }

        // --- Getters ---

        [[nodiscard]] GridIndexType rows() const noexcept { return rows_; }
        [[nodiscard]] GridIndexType cols() const noexcept { return cols_; }
        [[nodiscard]] FlatVectorIndexType cellCount() const noexcept { return cellCount_; }
        [[nodiscard]] float cellSize() const noexcept { return cellSize_; }

        [[nodiscard]] float mapOriginX() const noexcept { return mapOriginX_; }
        [[nodiscard]] float mapOriginY() const noexcept { return mapOriginY_; }

        // --- Static Metadata Access ---

        static const LayerDescriptor& getDescriptor(LayerId id);

    private:
        GridIndexType rows_ = 0;
        GridIndexType cols_ = 0;
        FlatVectorIndexType cellCount_ = 0;
        float cellSize_ = 0.0f;

        // Storage for all layers defined in the enum
        std::array<LayerDataVariant, magic_enum::enum_count<LayerId>()> layers_;

        float mapOriginX_;
        float mapOriginY_;


        // Metadata descriptors (static)
        static const std::array<LayerDescriptor, magic_enum::enum_count<LayerId>()> descriptors_;

        // Helper for static initialization
        static std::array<LayerDescriptor, magic_enum::enum_count<LayerId>()> initializeDescriptors();
    };

} // namespace danasim
