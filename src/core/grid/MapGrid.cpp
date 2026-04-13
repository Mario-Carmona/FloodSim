/**
 * @file MapGrid.cpp
 * @brief Implementation of the MapGrid and static descriptor initialization.
 */

#include "core/grid/MapGrid.hpp"

#include <algorithm>
#include <cstring>
#include <cmath>
#include <stdexcept>

#include "logging/Logger.hpp"

namespace danasim {

    MapGrid::MapGrid()
    {
        
    }

    void MapGrid::load(const ModelParamsInfo& paramsInfo, InputPort* mainInputSource,
            const std::unordered_map<std::string, InputPort*>& layersAlternativeInputSource,
            const std::unordered_map<std::string, std::string>& scalarsConfig,
            std::chrono::seconds timeStep, std::chrono::system_clock::time_point currentTime) {
        
        LOG_INFO("Loading simulation layers...");


        metadata_ = {
            .cellCount = 0
        };

        for (const ModelParam& param : paramsInfo.layers) {

            InputPort* layerSource;

            if (layersAlternativeInputSource.contains(param.name)) {
                layerSource = layersAlternativeInputSource.at(param.name);
            }
            else {
                layerSource = mainInputSource;
            }

            bool isStaticLayer;

            if (param.loadRequired) {
                isStaticLayer = layerSource->isStaticLayer(param.name);
            }
            else {
                isStaticLayer = true;
            }

            switch (param.dataType) {
            case DataType::FLOAT32:
                addLayer<float>(param.name, isStaticLayer);
                break;
            case DataType::INT8:
                addLayer<int8_t>(param.name, isStaticLayer);
                break;
            }

            if (param.loadRequired) {
                GridMetadata layerMetadata = layerSource->generateReader(param.name, getLayer(param.name)->isStatic())->readMetadata();

                if (layerMetadata.cellCount > metadata_.cellCount) {
                    metadata_ = layerMetadata;
                }
            }
        }


        for (const ModelParam& param : paramsInfo.scalars) {
            switch (param.dataType) {
            case DataType::FLOAT32:
                addScalar<float>(param.name);
                break;
            case DataType::INT8:
                addScalar<int8_t>(param.name);
                break;
            }
        }


        for (const auto& [name, scalar] : scalars_) {
            if (name == "delta_x") {
                getScalar<float>(name)->setValue(metadata_.cellSize);
            }
            else if (name == "delta_t") {
                getScalar<float>(name)->setValue(static_cast<float>(timeStep.count()));
            }
            else {
                scalar->setValue(scalarsConfig.at(name));
            }
        }


        for (const ModelParam& param : paramsInfo.layers) {
            if (param.loadRequired) {
                InputPort* layerSource;

                if (layersAlternativeInputSource.contains(param.name)) {
                    layerSource = layersAlternativeInputSource.at(param.name);
                }
                else {
                    layerSource = mainInputSource;
                }

                LayerBase* layer = getLayer(param.name);

                std::unique_ptr<Reader> layerReader = layerSource->generateReader(layer->getName(), layer->isStatic());

                layer->setReader(metadata_, std::move(layerReader), currentTime);

                normalizeUnits(param.name);
            }
            else {
                LayerBase* layer = getLayer(param.name);

                layer->resize(metadata_.cellCount);
            }
        }
    }

    void MapGrid::updateDynamicLayers(std::chrono::system_clock::time_point currentTime) {
        for (const auto& [name, layer] : layers_) {
            if (!layer->isStatic()) {
                layer->update(currentTime);

                normalizeUnits(name);
            }
        }
    }

    void MapGrid::normalizeUnits(const std::string& name) {
        if (name == "rainfall") {
            constexpr float MM_TO_M_FACTOR = 0.001f;
            constexpr float HOUR_TO_SEC_INV = 1.0f / 3600.0f;

            // 1. Obtenemos el puntero base
            LayerBase* baseLayer = layers_[name].get();

            // 2. Casteamos a Layer<float> para poder acceder a getData()
            if (auto* floatLayer = dynamic_cast<Layer<float>*>(baseLayer)) {

                auto& data = floatLayer->getData(); // Ahora sí tenemos acceso al vector

                // 1. Extraemos el valor numérico de chrono y lo pasamos a float.
                // Lo hacemos FUERA del std::transform para que no se calcule 
                // repetidamente por cada celda de la malla.
                float timeStepSecs = getScalar<float>("delta_t")->getValue();

                std::transform(data.begin(), data.end(), data.begin(),
                    [MM_TO_M_FACTOR, HOUR_TO_SEC_INV, timeStepSecs](float val) {
                        return (val * MM_TO_M_FACTOR) * (timeStepSecs * HOUR_TO_SEC_INV);
                    });
            }
            else {
                LOG_ERROR("La capa Rainfall no es de tipo float");
            }
        }
    }

} // namespace danasim
