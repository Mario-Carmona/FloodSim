/**
 * @file MapGrid.cpp
 * @brief Implementation of the MapGrid and static descriptor initialization.
 */

#include "core/grid/MapGrid.hpp"

#include <algorithm>
#include <cstring>
#include <cmath>
#include <stdexcept>

#include "core/grid/StaticLayer.hpp"
#include "core/grid/DynamicLayer.hpp"

namespace danasim {

    MapGrid::MapGrid(std::chrono::seconds timeStep)
        : timeStep_(timeStep)
    {
        layers_[static_cast<size_t>(LayerId::TopoBathy)] = std::make_unique<StaticLayer<float>>("topo_bathy", LayerRole::Secondary);
        layers_[static_cast<size_t>(LayerId::WaterDepth)] = std::make_unique<StaticLayer<float>>("water_depth", LayerRole::Main);
        layers_[static_cast<size_t>(LayerId::LandCover)] = std::make_unique<StaticLayer<int8_t>>("land_cover", LayerRole::Secondary);
        layers_[static_cast<size_t>(LayerId::Rainfall)] = std::make_unique<DynamicLayer<float>>("rainfall", LayerRole::Secondary);

        for (LayerId layer : magic_enum::enum_values<LayerId>()) {
            if (layers_[static_cast<size_t>(layer)] == nullptr) {
                auto msg = fmt::format(
                    "La capa {} no ha sido cargada.",
                    magic_enum::enum_name(layer)
                );
                LOG_ERROR("{}", msg);
                throw std::runtime_error(msg);
            }
        }


        // ---------------------------------------------------------------------
        // 2. ORDER VALIDATION
        // ---------------------------------------------------------------------
        // These rules enforce the internal logic required by the Simulation Core.

        // RULE 1: The first layer (index 0) MUST be TopoBathy.
        // This is often required for base topology calculations.
        if (static_cast<LayerId>(0) != LayerId::TopoBathy) {
            std::string msg = "Layer Order Error: The first layer in LayerId enum MUST be 'TopoBathy'.";
            LOG_ERROR("{}", msg);
            throw std::logic_error(msg);
        }

        // RULE 2: Strict Grouping (Principal layers must appear before Secondary layers).
        // This optimizes memory access patterns and input loading loops.
        bool derivedStarted = false;

        for (LayerId layer : magic_enum::enum_values<LayerId>()) {
            if (getLayer(layer)->isDerived()) {
                // We have entered the Secondary layers zone
                derivedStarted = true;
            }
            else {
                // If we find a Principal layer after starting Secondary layers, it's an error.
                if (derivedStarted) {
                    auto msg = fmt::format(
                        "Layer Order Error: Layer '{}' (Non Derived) appears after a Derived layer. "
                        "Please reorder LayerId enum so all Non Derived layers come first.",
                        layers_[static_cast<size_t>(layer)]->getName()
                    );
                    LOG_ERROR("{}", msg);
                    throw std::logic_error(msg);
                }
            }
        }
    }

    void MapGrid::load(const std::unordered_map<std::string, InputPort*>& layerInputSource, std::chrono::system_clock::time_point currentTime) {
        LOG_INFO("Loading simulation layers...");

        std::string baseLayer = layers_[0]->getName();

        GridMetadata metadata = layerInputSource.at(baseLayer)->generateReader(baseLayer, getLayer(magic_enum::enum_value<LayerId>(0))->isStatic())->readMetadata();

        for (auto layerId : magic_enum::enum_values<LayerId>()) {
            try {
                getLayer(layerId)->init(metadata);

                if (getLayer(layerId)->isDerived()) {
                    initializeDerivedLayer(layerId);
                }
                else {
                    getLayer(layerId)->setReader(
                        layerInputSource.at(baseLayer)->generateReader(
                            getLayer(layerId)->getName(),
                            getLayer(layerId)->isStatic()
                        ),
                        currentTime
                    );

                    normalizeUnits(layerId);
                }
            }
            catch (const std::exception& ex) {
                // Enhance exception with context before re-throwing
                auto msg = fmt::format("Failed to load layer '{}': {}", getLayer(layerId)->getName(), ex.what());
                LOG_ERROR("{}", msg);
                throw std::runtime_error(msg);
            }
        }
    }

    void MapGrid::initializeDerivedLayer(LayerId id) {
        // Logic for derived layers.
        switch (id) {
        default: {
            auto msg = fmt::format(
                "There is no initialization logic for the secondary layer: {}",
                magic_enum::enum_name(id)
            );
            LOG_ERROR("{}", msg);
            throw std::runtime_error(msg);
            break;
        }
        }
    }

    void MapGrid::updateDynamicLayers(std::chrono::system_clock::time_point currentTime) {
        for (auto layerId : magic_enum::enum_values<LayerId>()) {
            if (!getLayer(layerId)->isStatic()) {
                getLayer(layerId)->update(currentTime);

                normalizeUnits(layerId);
            }
        }
    }

    void MapGrid::normalizeUnits(LayerId id) {
        switch (id) {
        case LayerId::Rainfall: {
            constexpr float MM_TO_M_FACTOR = 0.001f;
            constexpr float HOUR_TO_SEC_INV = 1.0f / 3600.0f;

            // 1. Obtenemos el puntero base
            LayerBase* baseLayer = getLayer(id);

            // 2. Casteamos a Layer<float> para poder acceder a getData()
            if (auto* floatLayer = dynamic_cast<Layer<float>*>(baseLayer)) {

                auto& data = floatLayer->getData(); // Ahora sí tenemos acceso al vector

                // 1. Extraemos el valor numérico de chrono y lo pasamos a float.
                // Lo hacemos FUERA del std::transform para que no se calcule 
                // repetidamente por cada celda de la malla.
                float timeStepSecs = static_cast<float>(timeStep_.count());

                std::transform(data.begin(), data.end(), data.begin(),
                    [MM_TO_M_FACTOR, HOUR_TO_SEC_INV, timeStepSecs](float val) {
                        return (val * MM_TO_M_FACTOR) * (timeStepSecs * HOUR_TO_SEC_INV);
                    });
            }
            else {
                LOG_ERROR("La capa Rainfall no es de tipo float");
            }
            break;
        }
        }
    }

    LayerBase* MapGrid::getLayer(LayerId id) {
        return layers_[static_cast<size_t>(id)].get();
    }

    const LayerBase* MapGrid::getLayer(LayerId id) const {
        return layers_[static_cast<size_t>(id)].get();
    }

} // namespace danasim
