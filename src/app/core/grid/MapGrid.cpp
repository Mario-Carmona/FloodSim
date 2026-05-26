/**
 * @file MapGrid.cpp
 * @brief Implementation of the MapGrid and static descriptor initialization.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/core/grid/MapGrid.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

#include <GeographicLib/UTMUPS.hpp>

#include "logging/Logger.hpp"

namespace floodsim {

void MapGrid::Load(const ModelParamsInfo& params_info,
                   InputPort* main_input_source,
                   const std::unordered_map<std::string, InputPort*>& layers_alternative_input_source,
                   const std::unordered_map<std::string, std::string>& scalars_config,
                   std::chrono::seconds time_step,
                   std::chrono::system_clock::time_point current_time) {

    LOG_INFO("Loading simulation layers...");

    // Zero-initialize metadata
    metadata_ = GridMetadata{};

    // 1. Initialize layers structure and determine global metadata
    for (const ModelParam& param : params_info.layers) {

        InputPort* layer_source = layers_alternative_input_source.contains(param.name)
            ? layers_alternative_input_source.at(param.name)
            : main_input_source;

        bool is_static_layer = true;
        if (param.load_required) {
            is_static_layer = layer_source->IsStaticLayer(param.name);
        }

        switch (param.data_type) {
        case DataType::kFloat32:
            AddLayer<float>(param.name, is_static_layer);
            break;
        case DataType::kInt8:
            AddLayer<int8_t>(param.name, is_static_layer);
            break;
        }

        if (param.load_required) {
            bool is_static = GetLayer(param.name)->IsStatic();
            GridMetadata layer_metadata = layer_source->GenerateReader(param.name, is_static)->ReadMetadata();

            if (layer_metadata.cell_count > metadata_.cell_count) {
                metadata_ = layer_metadata;
            }
        }
    }

    // 2. Initialize scalars
    for (const ModelParam& param : params_info.scalars) {
        switch (param.data_type) {
        case DataType::kFloat32:
            AddScalar<float>(param.name);
            break;
        case DataType::kInt8:
            AddScalar<int8_t>(param.name);
            break;
        }
    }

    // 3. Assign internal and configured scalar values
    for (const auto& [name, scalar] : scalars_) {
        if (name == "delta_x") {
            GetScalar<float>(name)->SetValue(metadata_.cell_size);
        }
        else if (name == "delta_t") {
            GetScalar<float>(name)->SetValue(static_cast<float>(time_step.count()));
        }
        else {
            scalar->SetValue(scalars_config.at(name));
        }
    }

    // 4. Load spatial data or allocate memory for all layers
    for (const ModelParam& param : params_info.layers) {
        LayerBase* layer = GetLayer(param.name);

        if (param.load_required) {
            InputPort* layer_source = layers_alternative_input_source.contains(param.name)
                ? layers_alternative_input_source.at(param.name)
                : main_input_source;

            std::unique_ptr<Reader> layer_reader = layer_source->GenerateReader(layer->GetName(), layer->IsStatic());

            layer->SetReader(metadata_, std::move(layer_reader), current_time);
            NormalizeUnits(param.name);
        }
        else {
            layer->Resize(metadata_.cell_count);
        }
    }
}

void MapGrid::UpdateDynamicLayers(std::chrono::system_clock::time_point current_time) {
    for (const auto& [name, layer] : layers_) {
        if (!layer->IsStatic()) {
            layer->Update(current_time);
            NormalizeUnits(name);
        }
    }
}

void MapGrid::NormalizeUnits(const std::string& name) {
    if (name == "rainfall") {
        constexpr float MM_TO_M_FACTOR = 0.001f;
        constexpr float HOUR_TO_SEC_INV = 1.0f / 3600.0f;

        // 1. Retrieve the type-erased base pointer
        LayerBase* base_layer = layers_[name].get();

        // 2. Safely downcast to Layer<float> to access the underlying data
        if (auto* float_layer = dynamic_cast<Layer<float>*>(base_layer)) {
            auto& data = float_layer->GetData();

            // Extract the time step scalar to a float.
            // Executed OUTSIDE the transform loop to prevent redundant evaluations.
            float time_step_secs = GetScalar<float>("delta_t")->GetValue();

            std::transform(data.begin(), data.end(), data.begin(),
                [MM_TO_M_FACTOR, HOUR_TO_SEC_INV, time_step_secs](float val) {
                    return (val * MM_TO_M_FACTOR) * (time_step_secs * HOUR_TO_SEC_INV);
                });
        }
        else {
            LOG_ERROR("MapGrid: Rainfall layer is not of type float.");
        }
    }
}

std::optional<UTMZoneInfo> MapGrid::ParseUTMZoneFromEPSG(const std::string& epsg_string) const {
    // 1. Extract only the numeric part (ignoring "EPSG:" prefix if present)
    size_t colon_pos = epsg_string.find(':');
    std::string num_part = (colon_pos != std::string::npos)
        ? epsg_string.substr(colon_pos + 1)
        : epsg_string;

    int epsg_code;
    try {
        epsg_code = std::stoi(num_part);
    }
    catch (...) {
        return std::nullopt; // Not a valid number
    }

    // 2. Extract the UTM zone (the last two digits)
    int zone = epsg_code % 100;

    // Basic UTM zone validation
    if (zone < 1 || zone > 60) {
        return std::nullopt;
    }

    // 3. Determine the geographic system and hemisphere
    int base_code = epsg_code - zone; // E.g., 25830 - 30 = 25800

    switch (base_code) {
    case 32600: // WGS 84 North
    case 25800: // ETRS89 North (Europe)
    case 23000: // ED50 North (Europe)
        return UTMZoneInfo{ zone, true };

    case 32700: // WGS 84 South
        return UTMZoneInfo{ zone, false };

    default:
        // A valid EPSG code numerically, but not from a recognized UTM block
        return std::nullopt;
    }
}

GridViewBox::Point MapGrid::TransformViewPoint(ViewBox::Point source_point, const std::string& target_crs) const {
    auto utm_info = ParseUTMZoneFromEPSG(target_crs);
    if (!utm_info.has_value()) {
        throw std::runtime_error("MapGrid: Destination CRS (" + target_crs + ") is not a mathematically supported UTM system.");
    }

    int computed_zone;
    bool computed_northp;
    GridViewBox::Point grid_view_point;

    try {
        // Enforce the dynamically extracted zone from the EPSG string
        GeographicLib::UTMUPS::Forward(source_point.lat, source_point.lon,
            computed_zone, computed_northp,
            grid_view_point.x, grid_view_point.y,
            utm_info->zone);

        // Verify that the actual coordinate falls within the expected hemisphere
        if (computed_northp != utm_info->is_north) {
            // Note: In some boundary cases (near the equator) this could be relaxed,
            // but strictly speaking, if the EPSG requires North and it falls South, there is a discrepancy.
            throw std::runtime_error("MapGrid: Coordinates fall in the wrong hemisphere for this EPSG projection.");
        }
    } catch (const GeographicLib::GeographicErr& e) {
        throw std::runtime_error(std::string("GeographicLib Error: ") + e.what());
    }

    return grid_view_point;
}

ViewBox::Point MapGrid::TransformGridViewPoint(GridViewBox::Point source_point, const std::string& target_crs) const {
    auto utm_info = ParseUTMZoneFromEPSG(target_crs);
    if (!utm_info.has_value()) {
        throw std::runtime_error("MapGrid: Destination CRS (" + target_crs + ") is not a mathematically supported UTM system.");
    }

    ViewBox::Point view_point;

    try {
        // Enforce the dynamically extracted zone and hemisphere
        GeographicLib::UTMUPS::Reverse(utm_info->zone, utm_info->is_north,
            source_point.x, source_point.y,
            view_point.lat, view_point.lon);
    }
    catch (const GeographicLib::GeographicErr& e) {
        throw std::runtime_error(std::string("GeographicLib Error: ") + e.what());
    }

    return view_point;
}

ViewBox::Point MapGrid::GetGeoreference() const {
    return TransformGridViewPoint(GridViewBox::Point{ metadata_.min_x, metadata_.max_y }, metadata_.crs);
}

} // namespace floodsim
