/**
 * @file OnnxStateUpdater.cpp
 * @brief Implementation of the AI-based physics state updater using ONNX Runtime.
 *
 * This file contains the implementation of the ONNX state updater. It handles
 * the initialization of the ONNX environment, thread-safe inference execution,
 * and robust exception management to ensure graceful degradation and safe
 * thread termination during abrupt shutdowns.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/adapters/state_updater/OnnxStateUpdater.hpp"

#include <algorithm>
#include <atomic>
#include <csignal>
#include <cstring>
#include <fstream>
#include <set>
#include <stdexcept>
#include <thread>
#include <vector>

#include <fmt/core.h>
#include <nlohmann/json.hpp>
#include <omp.h>

#include "logging/Logger.hpp"
#include "app/exception/Exception.hpp"

namespace floodsim::app::adapters::state_updater {

namespace {
    // Global atomic flag to signal abrupt termination to worker threads.
    std::atomic<bool> g_is_shutting_down{ false };
}

OnnxStateUpdater::OnnxStateUpdater(bool enable_rainfall, float dry_tolerance,
                                   const std::vector<config::FloodRiskLevel>& flood_risk_levels,
                                   const std::filesystem::path& model_path, int64_t tensor_dim)
    : core::ports::StateUpdaterPort(enable_rainfall, dry_tolerance, flood_risk_levels)
    , tensor_dim_(tensor_dim)
    , halo_size_(1)
    , tile_size_(tensor_dim_ - (2 * halo_size_))
    , env_(ORT_LOGGING_LEVEL_WARNING, FLOODSIM_PROGRAM_NAME)
    , memory_info_(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)) {

    try {
        Ort::SessionOptions session_options;

        unsigned int num_threads = std::thread::hardware_concurrency();

        // Fallback protection: If detection fails, force a minimum of 4 threads.
        if (num_threads == 0) {
            num_threads = 4;
        }

        // STRATEGY: Use all available cores minus 1 to avoid freezing the UI/OS.
        if (num_threads > 4) {
            num_threads -= 1;
        }

        LOG_INFO("Configuring ONNX session with {} threads.", num_threads);

        session_options.SetIntraOpNumThreads(num_threads);
        session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        session_options.EnableCpuMemArena();
        session_options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);
        session_options.EnableMemPattern();

        // Load models
        preprocess_session_ = Ort::Session(env_, (model_path / "preprocess_model.onnx").c_str(), session_options);
        LOG_INFO("ONNX Model loaded successfully from: {}", (model_path / "preprocess_model.onnx").string());

        step_session_ = Ort::Session(env_, (model_path / "step_model.onnx").c_str(), session_options);
        LOG_INFO("ONNX Model loaded successfully from: {}", (model_path / "step_model.onnx").string());

        std::ifstream file((model_path / "metadata.json").c_str());
        if (!file.is_open()) {
            LOG_ERROR("Error: Cannot open metadata.json file");
            throw floodsim::app::exception::FloodSimException("Failed to open metadata.json");
        }

        // Parse JSON
        nlohmann::json j;
        file >> j;

        fluid_layer_name_ = j["fluid_layer"].get<std::string>();
        fluid_movement_state_layer_name_ = j["fluid_movement_state_layer"].get<std::string>();

        preprocess_model_ = ParseModelGraphInfo(j["preprocess"]);
        step_model_ = ParseModelGraphInfo(j["step"]);

        std::set<std::string> preprocess_input_params;
        for (const auto& item : preprocess_model_.inputs) {
            preprocess_input_params.insert(item.id_name);
        }

        std::set<std::string> preprocess_output_params;
        for (const auto& item : preprocess_model_.outputs) {
            preprocess_output_params.insert(item.id_name);
        }

        std::set<std::string> step_input_params;
        for (const auto& item : step_model_.inputs) {
            step_input_params.insert(item.id_name);
        }

        for (const auto& item : preprocess_model_.inputs) {
            if (item.is_scalar) {
                params_info_.scalars.push_back({
                    .name = item.id_name,
                    .data_type = item.type,
                    .load_required = true
                    });
            }
            else {
                params_info_.layers.push_back({
                    .name = item.id_name,
                    .data_type = item.type,
                    .load_required = true
                    });
            }
        }

        for (const auto& item : preprocess_model_.outputs) {
            if (!preprocess_input_params.contains(item.id_name)) {
                if (item.is_scalar) {
                    params_info_.scalars.push_back({
                        .name = item.id_name,
                        .data_type = item.type,
                        .load_required = true
                        });
                }
                else {
                    params_info_.layers.push_back({
                        .name = item.id_name,
                        .data_type = item.type,
                        .load_required = false
                        });
                }
            }
        }

        for (const auto& item : step_model_.inputs) {
            if (!preprocess_input_params.contains(item.id_name) && !preprocess_output_params.contains(item.id_name)) {
                if (item.is_scalar) {
                    params_info_.scalars.push_back({
                        .name = item.id_name,
                        .data_type = item.type,
                        .load_required = true
                        });
                }
                else {
                    params_info_.layers.push_back({
                        .name = item.id_name,
                        .data_type = item.type,
                        .load_required = (!preprocess_output_params.contains(item.id_name))
                        });
                }
            }
        }

        for (const auto& item : step_model_.outputs) {
            if (!preprocess_input_params.contains(item.id_name) && !preprocess_output_params.contains(item.id_name) && !step_input_params.contains(item.id_name)) {
                if (item.is_scalar) {
                    params_info_.scalars.push_back({
                        .name = item.id_name,
                        .data_type = item.type,
                        .load_required = true
                        });
                }
                else {
                    params_info_.layers.push_back({
                        .name = item.id_name,
                        .data_type = item.type,
                        .load_required = false
                        });
                }
            }
        }

        if (IsRainfallEnabled()) {
            params_info_.layers.push_back({
                .name = std::string(kRainfallLayerName),
                .data_type = core::grid::DataType::kFloat32,
                .load_required = true
                });
        }

        params_info_.layers.push_back({
            .name = std::string(kFloodRiskLayerName),
            .data_type = core::grid::DataType::kInt8,
            .load_required = false
            });
    } catch (const Ort::Exception& e) {
        LOG_ERROR("ONNX Runtime initialization failed: {}", e.what());
        throw floodsim::app::exception::FloodSimException(fmt::format("OnnxStateUpdater init error: {}", e.what()));
    } catch (const std::exception& e) {
        LOG_ERROR("Standard exception during ONNX initialization: {}", e.what());
        throw;
    }
}

int8_t OnnxStateUpdater::ClassifyRisk(float depth) const {
    const std::vector<config::FloodRiskLevel>& flood_risk_levels = GetFloodRiskLevels();

    int8_t level = 0;

    for (std::size_t i = 0; i < flood_risk_levels.size(); ++i) {
        if (depth >= flood_risk_levels[i].threshold_start) {
			level = static_cast<int8_t>(i);
        }
        else {
            return level;
        }
    }

    return level;
}

void OnnxStateUpdater::RunModel(Ort::Session& session, core::grid::MapGrid& grid,
                                const Ort::RunOptions& options,
                                const ModelGraphInfo& graph_info,
                                const std::vector<int64_t>& tensor_shape,
                                bool use_dynamic_bounding_box,
                                const std::vector<core::grid::layers::Tile>& active_tiles) {

    try {
        size_t tensor_size = 1;
        for (int64_t item : tensor_shape) {
            tensor_size *= item;
        }

        if (use_dynamic_bounding_box) {
            core::grid::layers::ScratchpadBase* first_scratchpad = layers_scratchpad_.begin()->second.get();

            if (tensor_size > first_scratchpad->GetSize()) {
                size_t new_size;

                if (first_scratchpad->GetSize() == 0) {
                new_size = std::min(
                    static_cast<size_t>(tensor_size * 1.5f),
                    max_tensor_elements_  
				);
                }
                else {
                new_size = std::min(
                    static_cast<size_t>(first_scratchpad->GetSize() * 1.5f),
                    max_tensor_elements_    
                );
                }

                for (const auto& [name, scratchpad] : layers_scratchpad_) {
                    scratchpad->Resize(new_size);
                }
            }

            for (const auto& item : graph_info.inputs) {
                if (!item.is_scalar) {
                    grid.GetLayer(item.id_name)->ExtractTilesData(layers_scratchpad_[item.id_name].get(), active_tiles, grid.GetCols(), grid.GetRows(),
                        halo_size_, tensor_dim_, tensor_dim_);
                } 
            }
        }

        Ort::IoBinding io_binding(session);

        for (const auto& item : graph_info.inputs) {
            io_binding.BindInput(
                item.model_name.c_str(), 
                ConfigureTensor(
                    item, grid, tensor_shape, tensor_size, use_dynamic_bounding_box
                )
            );
        }

        for (const auto& item : graph_info.outputs) {
            io_binding.BindOutput(
                item.model_name.c_str(),
                ConfigureTensor(
                    item, grid, tensor_shape, tensor_size, use_dynamic_bounding_box
                )
            );
        }

        session.Run(options, io_binding);

        if (use_dynamic_bounding_box) {
            for (const auto& item : graph_info.outputs) {
                if (!item.is_scalar) {
                    grid.GetLayer(item.id_name)->UpdateTilesData(layers_scratchpad_[item.id_name].get(), active_tiles, grid.GetCols(),
                        halo_size_, tensor_dim_, tensor_dim_);
                }
            }
        }
    }
    catch (const Ort::Exception& e) {
        LOG_ERROR("ONNX Inference Error: {}", e.what());
    }
}

    void OnnxStateUpdater::Initialize(core::grid::MapGrid& grid) {
        int64_t num_tiles_x = (grid.GetCols() + tile_size_ - 1) / tile_size_;
        int64_t num_tiles_y = (grid.GetRows() + tile_size_ - 1) / tile_size_;

        int64_t max_active_tiles = num_tiles_x * num_tiles_y;
        max_tensor_elements_ = max_active_tiles * tensor_dim_ * tensor_dim_;


        for (const auto& item : step_model_.inputs) {
            if (!item.is_scalar) {
                layers_scratchpad_[item.id_name] = grid.GetLayer(item.id_name)->GenerateScratchpad();
            }
        }

        for (const auto& item : step_model_.outputs) {
            if (!item.is_scalar) {
                if (layers_scratchpad_.find(item.id_name) == layers_scratchpad_.end()) {
                    layers_scratchpad_[item.id_name] = grid.GetLayer(item.id_name)->GenerateScratchpad();
                }
            }
        }

        int64_t rows = static_cast<int64_t>(grid.GetRows());
        int64_t cols = static_cast<int64_t>(grid.GetCols());

        std::vector<int64_t> tensor_shape = { rows, cols };
        std::vector<core::grid::layers::Tile> active_tiles;
        Ort::RunOptions preprocess_options{ nullptr };

        RunModel(preprocess_session_, grid, preprocess_options, preprocess_model_, tensor_shape, false, active_tiles);

        std::set<std::string> step_input_params;
        for (const auto& item : step_model_.inputs) {
            step_input_params.insert(item.id_name);
        }

        for (const auto& item : preprocess_model_.inputs) {
            if (!step_input_params.contains(item.id_name)) {
                grid.GetLayer(item.id_name)->Clear();
            }
        }

        const std::vector<float>& fluid_depth = grid.GetLayer<float>(GetFluidLayer())->GetData();
        std::vector<int8_t>& flood_risk = grid.GetLayer<int8_t>(std::string(kFloodRiskLayerName))->GetData();
        std::vector<int8_t>& fluid_movement_state = grid.GetLayer<int8_t>(GetFluidMovementStateLayer())->GetData();

        #pragma omp parallel for
        for (int i = 0; i < grid.GetMetadata().cell_count; ++i) {
            flood_risk[i] = ClassifyRisk(fluid_depth[i]);

            if (fluid_depth[i] >= GetDryTolerance()) {
                fluid_movement_state[i] = static_cast<int8_t>(WaterMovementState::kDynamicState);
            }
            else {
                fluid_movement_state[i] = static_cast<int8_t>(WaterMovementState::kStaticState);
            }
        }
    }

    void OnnxStateUpdater::GetActiveTiles(const core::grid::MapGrid& grid, std::vector<core::grid::layers::Tile>& active_tiles) const {
        int64_t cols = grid.GetCols();
        int64_t rows = grid.GetRows();

        int64_t num_tiles_x = (cols + tile_size_ - 1) / tile_size_;
        int64_t num_tiles_y = (rows + tile_size_ - 1) / tile_size_;

        const int8_t* fluid_movement_state = grid.GetLayer<int8_t>(GetFluidMovementStateLayer())->GetData().data();

        for (int64_t ty = 0; ty < num_tiles_y; ++ty) {
            for (int64_t tx = 0; tx < num_tiles_x; ++tx) {

                int64_t start_x = tx * tile_size_;
                int64_t start_y = ty * tile_size_;

                int64_t width = std::min(tile_size_, cols - start_x);
                int64_t height = std::min(tile_size_, rows - start_y);

                int64_t check_start_x = std::max(int64_t(0), start_x - halo_size_);
                int64_t check_start_y = std::max(int64_t(0), start_y - halo_size_);
                int64_t check_end_x = std::min(cols, start_x + width + halo_size_);
                int64_t check_end_y = std::min(rows, start_y + height + halo_size_);

                int64_t check_width = check_end_x - check_start_x;
                bool tile_is_active = false;

                for (int64_t y = check_start_y; y < check_end_y && !tile_is_active; ++y) {
                    int64_t row_start_idx = (y * cols) + check_start_x;
                    const int8_t* fluid_movement_row = fluid_movement_state + row_start_idx;

                    for (int64_t x = 0; x < check_width && !tile_is_active; ++x) {
                        if (fluid_movement_row[x] == static_cast<int8_t>(WaterMovementState::kDynamicState)) {
                            tile_is_active = true;
                        }
                    }
                }

                if (tile_is_active) {
                    active_tiles.push_back({ start_x, start_y, width, height });
                }
            }
        }
    }

    void OnnxStateUpdater::Step(core::grid::MapGrid& grid) {
        try {
            std::vector<float>& water_depth = grid.GetLayer<float>(GetFluidLayer())->GetData();
            std::vector<int8_t>& fluid_movement_state = grid.GetLayer<int8_t>(GetFluidMovementStateLayer())->GetData();

            if (IsRainfallEnabled()) {
                const std::vector<float>& rainfall = grid.GetLayer<float>(std::string(kRainfallLayerName))->GetData();

                #pragma omp parallel for
                for (size_t i = 0; i < water_depth.size(); ++i) {
                    water_depth[i] += rainfall[i];

                    if (water_depth[i] >= GetDryTolerance()) {
                        fluid_movement_state[i] = static_cast<int8_t>(WaterMovementState::kDynamicState);
                    }
                }
            }
                
            std::vector<core::grid::layers::Tile> active_tiles;
            GetActiveTiles(grid, active_tiles);

            int64_t batch_size = active_tiles.size();

            if (batch_size == 0) {
                LOG_WARN("No active water detected. Skipping ONNX inference.");
                return;
            }

            LOG_DEBUG("Active tiles (Batch Size) for ONNX: {}", batch_size);
            std::vector<int64_t> tensor_shape = { batch_size, tensor_dim_, tensor_dim_ };
            Ort::RunOptions step_options { nullptr };

            RunModel(step_session_, grid, step_options, step_model_, tensor_shape, true, active_tiles);

            std::vector<int8_t>& flood_risk = grid.GetLayer<int8_t>(std::string(kFloodRiskLayerName))->GetData();

            #pragma omp parallel for
            for (int64_t b = 0; b < static_cast<int64_t>(active_tiles.size()); ++b) {
                const auto& tile = active_tiles[b];

                for (int64_t core_y = 0; core_y < tile.core_height; ++core_y) {

                    int64_t global_y = tile.core_start_y + core_y;
                    int64_t global_offset = (global_y * grid.GetCols()) + tile.core_start_x;

                    for (int64_t i = 0; i < tile.core_width; i++) {
                        int64_t elem = global_offset + i;

                        flood_risk[elem] = ClassifyRisk(water_depth[elem]);
                    }
                }
            }
        }
        catch (const Ort::Exception& e) {
            LOG_ERROR("ONNX Inference Error: {}", e.what());
        }
    }

    TensorInfo OnnxStateUpdater::ParseTensorInfo(const nlohmann::json& tensor_json, bool output_tensor) {
        TensorInfo tensor;

        tensor.model_name = tensor_json["name"].get<std::string>();

        if (output_tensor) {
            size_t pos = tensor.model_name.find(":out");

            if (pos != std::string::npos) {
                tensor.id_name = tensor.model_name.substr(0, pos);
            }
            else {
                throw floodsim::app::exception::FloodSimException("Error: Output tensor name '" + tensor.model_name + "' is malformed (expected to contain ':out').");
            }
        }
        else {
            tensor.id_name = tensor.model_name;
        }

        if (tensor_json["type"] == "FLOAT") {
            tensor.type = core::grid::DataType::kFloat32;
        }
        else if (tensor_json["type"] == "INT8") {
            tensor.type = core::grid::DataType::kInt8;
        }
        else {
            throw floodsim::app::exception::FloodSimException("Error: ONNX tensor type '" + std::string(tensor_json["type"]) + "' is unsupported or missing C++ mapping.");
        }

        tensor.is_scalar = (tensor_json["shape"].size() == 0);

        return tensor;
    }

    ModelGraphInfo OnnxStateUpdater::ParseModelGraphInfo(const nlohmann::json& graph_json) {
        ModelGraphInfo graph_info;

        if (graph_json.contains("inputs")) {
            for (const auto& item : graph_json["inputs"]) {
                graph_info.inputs.push_back(ParseTensorInfo(item, false));
            }
        }

        if (graph_json.contains("outputs")) {
            for (const auto& item : graph_json["outputs"]) {
                graph_info.outputs.push_back(ParseTensorInfo(item, true));
            }
        }

        return graph_info;
    }

    Ort::Value OnnxStateUpdater::ConfigureTensor(const TensorInfo& info, core::grid::MapGrid& grid, const std::vector<int64_t>& tensor_shape, size_t tensor_size, bool use_dynamic_bounding_box) {
        if (info.is_scalar) {
            std::vector<int64_t> scalar_shape = {};

            switch (info.type) {
            case core::grid::DataType::kFloat32:
                return Ort::Value::CreateTensor<float>(memory_info_, &grid.GetScalar<float>(info.id_name)->GetValue(), 1, scalar_shape.data(), scalar_shape.size());
                break;
            case core::grid::DataType::kInt8:
                return Ort::Value::CreateTensor<int8_t>(memory_info_, &grid.GetScalar<int8_t>(info.id_name)->GetValue(), 1, scalar_shape.data(), scalar_shape.size());
                break;
            }
        }
        else {
            switch (info.type) {
            case core::grid::DataType::kFloat32:
                float* float_layer_data_prt;

                if (use_dynamic_bounding_box) {
                    float_layer_data_prt = dynamic_cast<core::grid::layers::Scratchpad<float>*>(layers_scratchpad_[info.id_name].get())->GetData().data();
                }
                else {
                    float_layer_data_prt = grid.GetLayer<float>(info.id_name)->GetData().data();
                }

                return Ort::Value::CreateTensor<float>(
                    memory_info_, 
                    float_layer_data_prt,
                    tensor_size, tensor_shape.data(), tensor_shape.size()
                );
                break;
            case core::grid::DataType::kInt8:
                int8_t* int8_layer_data_prt;

                if (use_dynamic_bounding_box) {
                    int8_layer_data_prt = dynamic_cast<core::grid::layers::Scratchpad<int8_t>*>(layers_scratchpad_[info.id_name].get())->GetData().data();
                }
                else {
                    int8_layer_data_prt = grid.GetLayer<int8_t>(info.id_name)->GetData().data();
                }

                return Ort::Value::CreateTensor<int8_t>(
                    memory_info_, 
                    int8_layer_data_prt,
                    tensor_size, tensor_shape.data(), tensor_shape.size()
                );
                break;
            }
        }

        throw floodsim::app::exception::FloodSimException("Unsupported data type in ConfigureTensor: " + info.id_name);
    }

} // namespace floodsim::app::adapters::state_updater
