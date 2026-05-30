/**
 * @file OnnxStateUpdater.hpp
 * @brief AI-based physics state updater using ONNX Runtime.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>
#include <onnxruntime_cxx_api.h>

#include "app/core/grid/MapGrid.hpp"
#include "app/core/ports/StateUpdaterPort.hpp"

namespace floodsim::app::adapters::state_updater {

/**
 * @brief Describes a tensor input/output parameter mapping.
 */
struct TensorInfo {
    /** @brief The identifier name of the tensor in the ONNX model. */
    std::string model_name;
    /** @brief The corresponding layer name in the grid or scratchpad. */
    std::string id_name;
    /** @brief The data type of the tensor (e.g., float32, int8). */
    core::grid::DataType type;
    /** @brief Flag indicating if the tensor is a scalar (single value) or a spatial grid. */
    bool is_scalar;
};

/**
 * @brief Holds structural mappings for an ONNX model graph.
 */
struct ModelGraphInfo {
    /** @brief Vector of TensorInfo describing the model's input tensors. */
    std::vector<TensorInfo> inputs;
    /** @brief Vector of TensorInfo describing the model's output tensors. */
    std::vector<TensorInfo> outputs;
};

/**
 * @brief ONNX Neural Operator adapter for advancing fluid simulation states.
 */
class OnnxStateUpdater : public core::ports::StateUpdaterPort {
public:
    /**
     * @brief Initializes the ONNX runtime environment and model sessions.
     * @param enable_rainfall Flag to incorporate atmospheric precipitation data.
     * @param dry_tolerance Height threshold to consider a cell as dry.
     * @param flood_risk_levels Array mapping water height to risk categories.
     * @param model_path Path to the trained .onnx network file.
     * @param tensor_dim Square spatial dimension size for the model tensors.
     * @throws std::runtime_error If the ONNX session fails to initialize.
     */
    explicit OnnxStateUpdater(bool enable_rainfall, float dry_tolerance,
        const std::vector<config::FloodRiskLevel>& flood_risk_levels,
        const std::filesystem::path& model_path,
        int64_t tensor_dim);

    ~OnnxStateUpdater() override = default;
    
    /**
     * @brief Pushes static topological data (Elevation, Roughness, dx, dt) 
     * into the model context. Call once before simulation loop.
     * @param grid Reference to the domain map structure.
     */
    void Initialize(core::grid::MapGrid& grid) override;

    /**
     * @brief Runs an inference step to advance fluid dynamics by one `dt`.
     * @param grid Reference to the domain map structure containing current state.
     */
    void Step(core::grid::MapGrid& grid) override;

    /**
   * @brief Retrieves internal physical parameters of the parsed model.
   * @return Constant reference to ModelParamsInfo.
   */
    const core::grid::ModelParamsInfo& GetModelParamsInfo() const override {
        return params_info_;
    }

    /**
     * @brief Returns the identifier name for the active fluid layer.
     * @return Constant reference to the layer name string.
     */
    const std::string& GetFluidLayer() const override {
        return fluid_layer_name_;
    }

    /**
     * @brief Returns the identifier name for the fluid movement state mask.
     * @return Constant reference to the movement state layer name string.
     */
    const std::string& GetFluidMovementStateLayer() const override {
        return fluid_movement_state_layer_name_;
    }

private:
    int64_t tensor_dim_;
    int64_t halo_size_;
    int64_t tile_size_;
    size_t max_tensor_elements_;

    core::grid::ModelParamsInfo params_info_;
    ModelGraphInfo preprocess_model_;
    ModelGraphInfo step_model_;

    std::string fluid_layer_name_;
    std::string fluid_movement_state_layer_name_;

    // Scratchpads for holding memory buffers aligned for ONNX inference.
    std::unordered_map<std::string, std::unique_ptr<core::grid::layers::ScratchpadBase>> layers_scratchpad_;

    // ONNX Runtime Environment
    Ort::Env env_;
    Ort::MemoryInfo memory_info_{ nullptr };
    Ort::Session preprocess_session_{ nullptr };
    Ort::Session step_session_{ nullptr };

    /**
     * @brief Dispatches the model execution on the ONNX graph.
     */
    void RunModel(Ort::Session& session, core::grid::MapGrid& grid,
        const Ort::RunOptions& options,
        const ModelGraphInfo& graph_info,
        const std::vector<int64_t>& tensor_shape,
        bool use_dynamic_bounding_box,
        const std::vector<core::grid::layers::Tile>& active_tiles);

    /**
     * @brief Scans the grid for tiles containing active dynamic fluid.
     */
    void GetActiveTiles(const core::grid::MapGrid& grid,
        std::vector<core::grid::layers::Tile>& active_tiles) const;

    /**
     * @brief Extracts tensor definitions from the accompanying JSON metadata.
     */
    ModelGraphInfo ParseModelGraphInfo(const nlohmann::json& graph_json);

    /**
     * @brief Binds a grid layer or scratchpad to an ONNX Ort::Value tensor representation.
     */
    Ort::Value ConfigureTensor(const TensorInfo& info, core::grid::MapGrid& grid,
        const std::vector<int64_t>& tensor_shape,
        size_t tensor_size,
        bool use_dynamic_bounding_box);
    
    /**
	 * @brief Parses a single tensor's metadata from the JSON configuration.
	 * @param tensor_json The JSON object containing the tensor's metadata.
	 * @param output_tensor Flag indicating if the tensor is an output (affects naming conventions).
	 * @return TensorInfo Struct containing the parsed tensor information.
     */
    TensorInfo ParseTensorInfo(const nlohmann::json& tensor_json, bool output_tensor);

    /**
	 * @brief Classifies the risk level based on fluid depth.
	 * @param depth The fluid depth.
	 * @return The risk level.
     */
    int8_t ClassifyRisk(float depth) const;
};

} // namespace floodsim::app::adapters::state_updater
