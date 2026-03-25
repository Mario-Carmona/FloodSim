
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <onnxruntime_cxx_api.h>

#include "ports/StateUpdaterPort.hpp"
#include "core/grid/MapGrid.hpp"

namespace danasim {

    class OnnxStateUpdater : public StateUpdaterPort {
    public:
        explicit OnnxStateUpdater(const std::filesystem::path& modelPath);
        ~OnnxStateUpdater() = default;

        // Método para enviar datos estáticos (Elevación, Rugosidad, dt, dx) una sola vez
        void initialize(MapGrid& grid, std::chrono::seconds stepTime) override;

        void run() override;

    private:
        // ONNX Runtime Environment
        Ort::Env env_;
        Ort::Session init_session_{ nullptr };
        Ort::Session run_session_{ nullptr };
        Ort::MemoryInfo memory_info_{ nullptr };

        Ort::RunOptions init_options_{ nullptr };
        Ort::RunOptions run_options_{ nullptr };

        std::vector<Ort::Value> init_input_tensors_;
        std::vector<Ort::Value> init_output_tensors_;

        std::vector<Ort::Value> run_input_tensors_;
        std::vector<Ort::Value> run_output_tensors_;

        // Nombres de los nodos del grafo (Deben coincidir con export_model.py)
        const std::vector<const char*> init_input_names_ = { "land_cover", "dt", "dx" };
        const std::vector<const char*> init_output_names_ = { "k_spatial" };

        const std::vector<const char*> run_input_names_ = { "water_depth", "topo_bathy", "k_spatial", "rainfall" };
        const std::vector<const char*> run_output_names_ = { "new_water_depth" }; // tf2onnx suele llamar al return 'output_0' o el nombre del nodo

        // Buffer de salida temporal (necesario porque ONNX no garantiza In-Place seguro en todos los ops)
        float dt_;
        float dx_;

        std::vector<float> k_spatial;
    };

} // namespace danasim
