
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
        void initialize(MapGrid& grid, float step_time) override;

        void run() override;

    private:
        // ONNX Runtime Environment
        Ort::Env env_;
        Ort::Session session_{ nullptr };
        Ort::MemoryInfo memory_info_{ nullptr };
        Ort::RunOptions run_options_{ nullptr };

        std::vector<Ort::Value> input_tensors_;
        std::vector<Ort::Value> output_tensors_;

        // Nombres de los nodos del grafo (Deben coincidir con export_model.py)
        const std::vector<const char*> input_names_ = { "water_depth", "elevation", "roughness", "rainfall", "dt", "dx", "steps" };
        const std::vector<const char*> output_names_ = { "Identity:0" }; // tf2onnx suele llamar al return 'output_0' o el nombre del nodo

        // Buffer de salida temporal (necesario porque ONNX no garantiza In-Place seguro en todos los ops)
        float dt_;
        float dx_;
    };

} // namespace danasim
