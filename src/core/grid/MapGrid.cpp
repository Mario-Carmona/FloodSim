
#include "core/grid/MapGrid.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace danasim {

    // -------------------------------------------------------------------------
    // Inicialización Estática de Descriptores
    // -------------------------------------------------------------------------
    std::array<LayerDescriptor, magic_enum::enum_count<LayerId>()> MapGrid::initializeDescriptors() {
        std::array<LayerDescriptor, magic_enum::enum_count<LayerId>()> buffer;

        // Recorremos TODOS los valores del enum automáticamente
        for (auto& value : magic_enum::enum_values<LayerId>()) {
            size_t index = static_cast<size_t>(value);

            // Aquí definimos las propiedades. 
            // Si añades un LayerId y no lo pones aquí, saltará el 'default' con error.
            switch (value) {
            case LayerId::Elevation:
                buffer[index] = { "elevation", LayerCategory::Static, LayerRole::Principal };
                break;
            case LayerId::Roughness:
                buffer[index] = { "roughness", LayerCategory::Static, LayerRole::Principal };
                break;
            case LayerId::WaterDepth:
                buffer[index] = { "water_depth", LayerCategory::Dynamic, LayerRole::Principal };
                break;
            case LayerId::CellState:
                buffer[index] = { "cell_state", LayerCategory::Dynamic, LayerRole::Secondary };
                break;
                // ... añade tus otros LayerId aquí ...

            default:
                // ERROR DE SEGURIDAD EN TIEMPO DE EJECUCIÓN (AL INICIO)
                std::string errorMsg = "MapGrid Fatal Error: LayerId::";
                errorMsg += magic_enum::enum_name(value);
                errorMsg += " no tiene un LayerDescriptor definido en MapGrid.cpp";
                throw std::logic_error(errorMsg);
            }
        }
        return buffer;
    }

    // Instanciación de la variable estática
    const std::array<LayerDescriptor, magic_enum::enum_count<LayerId>()> MapGrid::descriptors_ = MapGrid::initializeDescriptors();
    

    // -------------------------------------------------------------------------
    // Implementación de MapGrid
    // -------------------------------------------------------------------------

    MapGrid::MapGrid() = default; // El compilador inicializa state_ y layers_ vacíos

    void MapGrid::init(uint32_t w, uint32_t h, float cellSize) {
        width_ = w;
        height_ = h;
        cellCount_ = w * h;
        cellSize_ = cellSize;

        // Limpiamos vectores de cambios
        changed_x_.clear();
        changed_y_.clear();

        // Asignamos memoria para cada capa
        for (auto& layer : layers_) {
            // .assign redimensiona y llena de ceros (muy rápido)
            layer.data.assign(cellCount_, 0.0f);
        }
    }

    float* MapGrid::layerData(LayerId id) {
        // Acceso seguro: el array static garantiza que el índice existe si el Enum es válido
        return layers_[static_cast<size_t>(id)].data.data();
    }

    const float* MapGrid::layerData(LayerId id) const {
        return layers_[static_cast<size_t>(id)].data.data();
    }

    const LayerDescriptor& MapGrid::getDescriptor(LayerId id) {
        return descriptors_[static_cast<size_t>(id)];
    }

} // namespace danasim
