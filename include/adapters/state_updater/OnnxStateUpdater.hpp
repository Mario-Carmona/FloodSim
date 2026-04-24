
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <onnxruntime_cxx_api.h>
#include <nlohmann/json.hpp>

#include "ports/StateUpdaterPort.hpp"
#include "core/grid/MapGrid.hpp"

namespace danasim {

    struct TensorInfo {
        std::string model_name;
        std::string id_name;
        DataType type;
        bool isScalar;
    };


    struct ModelGraphInfo {
        std::vector<TensorInfo> inputs;
        std::vector<TensorInfo> outputs;
    };


    class OnnxStateUpdater : public StateUpdaterPort {
    public:
        explicit OnnxStateUpdater(bool enableRainfall, float dryTolerance, const std::vector<FloodRiskLevel>& floodRiskLevels, const std::filesystem::path& modelPath, int64_t tensorDim);
        ~OnnxStateUpdater() = default;

        // Método para enviar datos estáticos (Elevación, Rugosidad, dt, dx) una sola vez
        void initialize(MapGrid& grid) override;

        void step(MapGrid& grid) override;

        const ModelParamsInfo& getModelParamsInfo() const override { return paramsInfo_; };

        const std::string& getFluidLayer() const override { return fluidLayerName_; };
		const std::string& getFluidMovementStateLayer() const override { return fluidMovementStateLayerName_; };

    private:
        int64_t tensorDim_;
        int64_t haloSize_;
        int64_t tileSize_;

        size_t max_tensor_elements_;

        ModelParamsInfo paramsInfo_;

        ModelGraphInfo preprocessModel_;
        ModelGraphInfo stepModel_;

        std::string fluidLayerName_;
        std::string fluidMovementStateLayerName_;

        std::unordered_map<std::string, std::unique_ptr<ScratchpadBase>> layersScratchpad;

        // ONNX Runtime Environment
        Ort::Env env_;
        Ort::MemoryInfo memory_info_{ nullptr };

        Ort::Session preprocess_session_{ nullptr };
        Ort::Session step_session_{ nullptr };

        void runModel(Ort::Session& session, MapGrid& grid, const Ort::RunOptions& options,
            const ModelGraphInfo& graphInfo, const std::vector<int64_t>& tensor_shape, bool useDynamicBoundingBox,
            const std::vector<Tile>& active_tiles);

        void getActiveTiles(const MapGrid& grid, std::vector<Tile>& active_tiles) const;

        ModelGraphInfo parseModelGraphInfo(const nlohmann::json& graphJson);

        Ort::Value configureTensor(const TensorInfo& info, MapGrid& grid, const std::vector<int64_t>& tensor_shape, size_t tensor_size, bool useDynamicBoundingBox);
    
        TensorInfo parseTensorInfo(const nlohmann::json& tensorJson, bool outputTensor);

        int8_t classifyRisk(float depth) const;
    };


} // namespace danasim
