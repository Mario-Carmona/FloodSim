
#pragma once

#include <vector>
#include <array>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <magic_enum/magic_enum.hpp>

#include "core/grid/LayerTypes.hpp"

namespace danasim {

    // Descriptor simple sin ID (el índice del array será el ID)
    struct LayerDescriptor {
        std::string name;
        LayerCategory category;
        LayerRole role;
    };

    // Almacenamiento por valor (más eficiente que unique_ptr)
    struct LayerStorage {
        std::vector<float> data;

        // Constructor por defecto
        LayerStorage() = default;
        // Constructor con tamaño
        LayerStorage(size_t size) : data(size, 0.0f) {}
    };

    class MapGrid {
    public:
        MapGrid();

        // --- COPIA Y MOVIMIENTO AUTOMÁTICOS ---
        // Al usar vectores y structs, el compilador genera la versión más optimizada posible.
        // Esto soluciona tu problema original: si añades datos a GridState, 
        // se copiarán solos aquí.
        MapGrid(const MapGrid&) = default;
        MapGrid& operator=(const MapGrid&) = default;
        MapGrid(MapGrid&&) = default;
        MapGrid& operator=(MapGrid&&) = default;
        // --------------------------------------

        void init(uint32_t w, uint32_t h, float cellSize);

        // --- Getters ---
        uint32_t width() const { return width_; }
        uint32_t height() const { return height_; }
        uint32_t cellCount() const { return cellCount_; }
        float cellSize() const { return cellSize_; }

        // Acceso a datos crudos
        float* layerData(LayerId id);
        const float* layerData(LayerId id) const;

        // Acceso a metadatos de capa
        static const LayerDescriptor& getDescriptor(LayerId id);

        // Acceso a los vectores de cambios (Referencias directas)
        std::vector<int32_t>& changedX() { return changed_x_; }
        std::vector<int32_t>& changedY() { return changed_y_; }
        const std::vector<int32_t>& changedX() const { return changed_x_; }
        const std::vector<int32_t>& changedY() const { return changed_y_; }

    private:
        uint32_t width_ = 0;
        uint32_t height_ = 0;
        uint32_t cellCount_ = 0;
        float cellSize_ = 0.0f;

        // CAMBIO CLAVE: Almacenamos por valor, no por puntero.
        // std::vector se encarga de la memoria automáticamente.
        std::array<LayerStorage, magic_enum::enum_count<LayerId>()> layers_;

        // Solo guardamos coordenadas (Topology change)
        std::vector<int32_t> changed_x_;
        std::vector<int32_t> changed_y_;


        // Declaración del miembro estático y constante
        static const std::array<LayerDescriptor, magic_enum::enum_count<LayerId>()> descriptors_;

        // Función auxiliar privada para inicializar y validar
        static std::array<LayerDescriptor, magic_enum::enum_count<LayerId>()> initializeDescriptors();
    };

} // namespace danasim
