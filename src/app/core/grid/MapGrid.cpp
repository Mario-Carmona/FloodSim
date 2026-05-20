/**
 * @file MapGrid.cpp
 * @brief Implementation of the MapGrid and static descriptor initialization.
 */

#include "app/core/grid/MapGrid.hpp"

#include <algorithm>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <GeographicLib/UTMUPS.hpp>

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

    std::optional<UTMZoneInfo> MapGrid::parseUTMZoneFromEPSG(const std::string& epsgString) const {
        // 1. Extraer solo la parte numérica (ignorar "EPSG:" si está presente)
        size_t colonPos = epsgString.find(':');
        std::string numPart = (colonPos != std::string::npos) ?
            epsgString.substr(colonPos + 1) : epsgString;

        int epsgCode;
        try {
            epsgCode = std::stoi(numPart);
        }
        catch (...) {
            return std::nullopt; // No es un número válido
        }

        // 2. Extraer la zona (los dos últimos dígitos)
        int zone = epsgCode % 100;

        // Validación básica de zona UTM
        if (zone < 1 || zone > 60) {
            return std::nullopt;
        }

        // 3. Determinar el sistema y hemisferio
        int baseCode = epsgCode - zone; // Ej: 25830 - 30 = 25800

        switch (baseCode) {
        case 32600: // WGS 84 North
        case 25800: // ETRS89 North (Europa)
        case 23000: // ED50 North (Europa)
            return UTMZoneInfo{ zone, true };

        case 32700: // WGS 84 South
            return UTMZoneInfo{ zone, false };

        default:
            // Es un código EPSG válido numéricamente, pero no es de un bloque UTM reconocido
            return std::nullopt;
        }
    }

    GridViewBox::Point MapGrid::transformViewPoint(ViewBox::Point sourcePoint, const std::string& targetCRS) const {

        auto utmInfo = parseUTMZoneFromEPSG(targetCRS);
        if (!utmInfo.has_value()) {
            throw std::runtime_error("El CRS destino (" + targetCRS + ") no es un sistema UTM soportado matemáticamente.");
        }

        int computedZone;
        bool computedNorthp;

        GridViewBox::Point gridViewPoint;

        try {
            // Le pasamos la zona extraída dinámicamente del string (ej. 30)
            GeographicLib::UTMUPS::Forward(sourcePoint.lat, sourcePoint.lon, computedZone, computedNorthp, gridViewPoint.x, gridViewPoint.y, utmInfo->zone);

            // Verificamos que la coordenada real cae en el hemisferio que el EPSG espera
            if (computedNorthp != utmInfo->isNorth) {
                // Nota: En algunos casos limítrofes (cerca del ecuador) esto podría flexibilizarse,
                // pero estrictamente hablando, si el EPSG pide Norte y cae en Sur, hay una discrepancia.
                throw std::runtime_error("Las coordenadas caen en el hemisferio incorrecto para este EPSG.");
            }

        }
        catch (const GeographicLib::GeographicErr& e) {
            throw std::runtime_error(std::string("Error en GeographicLib: ") + e.what());
        }

        return gridViewPoint;
    }

    ViewBox::Point MapGrid::transformGridViewPoint(GridViewBox::Point sourcePoint, const std::string& targetCRS) const {

        auto utmInfo = parseUTMZoneFromEPSG(targetCRS);
        if (!utmInfo.has_value()) {
            throw std::runtime_error("El CRS destino (" + targetCRS + ") no es un sistema UTM soportado matemáticamente.");
        }

        int computedZone;
        bool computedNorthp;

        ViewBox::Point viewPoint;

        try {
            // Le pasamos la zona extraída dinámicamente del string (ej. 30)
            GeographicLib::UTMUPS::Reverse(
                utmInfo->zone,
                utmInfo->isNorth,
                sourcePoint.x,
                sourcePoint.y,
                viewPoint.lat,
                viewPoint.lon
            );

        }
        catch (const GeographicLib::GeographicErr& e) {
            throw std::runtime_error(std::string("Error en GeographicLib: ") + e.what());
        }

        return viewPoint;
    }

    ViewBox::Point MapGrid::georeference() const {
        return transformGridViewPoint(GridViewBox::Point{metadata_.minX, metadata_.maxY}, metadata_.crs);
    }

} // namespace danasim
