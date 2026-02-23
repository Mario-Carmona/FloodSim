/**
 * @file MapGrid.cpp
 * @brief Implementation of the MapGrid and static descriptor initialization.
 */

#include "core/grid/MapGrid.hpp"

#include <algorithm>
#include <cstring>
#include <cmath>
#include <stdexcept>

namespace danasim {

    // -------------------------------------------------------------------------
    // Static Initialization
    // -------------------------------------------------------------------------

    // We initialize the descriptors array once at startup.
    const std::array<LayerDescriptor, magic_enum::enum_count<LayerId>()> MapGrid::descriptors_ = MapGrid::initializeDescriptors();

    std::array<LayerDescriptor, magic_enum::enum_count<LayerId>()> MapGrid::initializeDescriptors() {
        std::array<LayerDescriptor, magic_enum::enum_count<LayerId>()> buffer;

        // ---------------------------------------------------------------------
        // 1. DESCRIPTOR ASSIGNMENT
        // ---------------------------------------------------------------------
        // We iterate over every value defined in the LayerId enum to fill the descriptors.

        for (auto& value : magic_enum::enum_values<LayerId>()) {
            size_t index = static_cast<size_t>(value);

            // Default initialization
            LayerDescriptor desc{};

            switch (value) {
                case LayerId::Elevation:
                    desc = {
                        "elevation", 
                        LayerRole::Secondary,
                        LayerDataType::Float32,
                        LayerSource::Static
                    };
                    break;
                case LayerId::Rainfall:
                    desc = {
                        "rainfall",
                        LayerRole::Secondary,
                        LayerDataType::Float32,
                        LayerSource::Dynamic
                    };
                    break;
                case LayerId::Roughness:
                    desc = {
                        "roughness",
                        LayerRole::Secondary,
                        LayerDataType::Float32,
                        LayerSource::Static
                    };
                    break;
                case LayerId::WaterDepth:
                    desc = {
                        "water_depth",
                        LayerRole::Main,
                        LayerDataType::Float32,
                        LayerSource::Static
                    };
                    break;
                    // ... añade tus otros LayerId aquí ...

                default:
                    // Critical safety catch for new enum values without descriptors
                    auto msg = fmt::format(
                        "LayerId '{}' is missing a descriptor definition in MapGrid::initializeDescriptors()",
                        magic_enum::enum_name(value)
                    );
                    LOG_ERROR("{}", msg);
                    throw std::logic_error(msg);
            }

            buffer[index] = desc;
        }

        // ---------------------------------------------------------------------
        // 2. ORDER VALIDATION
        // ---------------------------------------------------------------------
        // These rules enforce the internal logic required by the Simulation Core.

        // RULE 1: The first layer (index 0) MUST be Elevation.
        // This is often required for base topology calculations.
        if (static_cast<LayerId>(0) != LayerId::Elevation) {
            std::string msg = "Layer Order Error: The first layer in LayerId enum MUST be 'Elevation'.";
            LOG_ERROR("{}", msg);
            throw std::logic_error(msg);
        }

        // RULE 2: Strict Grouping (Principal layers must appear before Secondary layers).
        // This optimizes memory access patterns and input loading loops.
        bool secondaryStarted = false;

        for (size_t i = 0; i < buffer.size(); ++i) {
            const auto& desc = buffer[i];

            if (desc.source == LayerSource::Derived) {
                // We have entered the Secondary layers zone
                secondaryStarted = true;
            }
            else {
                // If we find a Principal layer after starting Secondary layers, it's an error.
                if (secondaryStarted) {
                    auto msg = fmt::format(
                        "Layer Order Error: Layer '{}' (Non Derived) appears after a Derived layer. "
                        "Please reorder LayerId enum so all Non Derived layers come first.",
                        desc.name
                    );
                    LOG_ERROR("{}", msg);
                    throw std::logic_error(msg);
                }
            }
        }

        return buffer;
    }

    const LayerDescriptor& MapGrid::getDescriptor(LayerId id) {
        return descriptors_[static_cast<size_t>(id)];
    }

    // -------------------------------------------------------------------------
    // Class Implementation
    // -------------------------------------------------------------------------

    void MapGrid::init(GridIndexType rows, GridIndexType cols, float cellSize, float mapOriginX, float mapOriginY) {

        // 1. Sanity Check: Ensure valid dimensions to prevent math errors later
        if (rows == 0 || cols == 0) {
            auto msg = fmt::format("MapGrid initialization failed: Invalid dimensions {}x{}.", cols, rows);
            LOG_ERROR("{}", msg);
            throw std::invalid_argument(msg);
        }

        // 2. Set Grid Metadata
        rows_ = rows;
        cols_ = cols;
        cellCount_ = rows_ * cols_;
        cellSize_ = cellSize;

        mapOriginX_ = mapOriginX;
        mapOriginY_ = mapOriginY;

        // 4. Allocate Memory for Layers
        // We iterate through all defined LayerIds using magic_enum to ensure
        // complete coverage. Vectors are re-allocated to the new cellCount.
        for (auto& layerId : magic_enum::enum_values<LayerId>()) {
            size_t index = static_cast<size_t>(layerId);
            const auto& desc = descriptors_[index];

            // Access the variant storage at this index
            auto& storage = layers_[index];

            // Initialize the variant based on the specific data type required 
            // by the layer descriptor (Float vs Int8).
            // Using std::vector constructor (size, value) to zero-initialize.
            if (desc.dataType == LayerDataType::Float32) {
                storage = std::vector<float>(cellCount_, 0.0f);
            }
            else if (desc.dataType == LayerDataType::Int8) {
                storage = std::vector<int8_t>(cellCount_, static_cast<int8_t>(0));
            }
        }
    }

} // namespace danasim
