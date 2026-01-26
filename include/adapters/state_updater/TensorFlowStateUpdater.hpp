
#pragma once

#include "core/ports/StateUpdaterPort.hpp"

#include <filesystem>

// === TENSORFLOW C API ===
extern "C" {
    #include "tensorflow/c/c_api.h"
}

namespace danasim {

    struct CachedLayer {
        TF_Output input_op;
        TF_Output output_op; // Solo para capas dinámicas
        LayerId id;
    };

    class TensorFlowStateUpdater final : public StateUpdaterPort {
    public:
        explicit TensorFlowStateUpdater(const std::filesystem::path& modelPath);
        ~TensorFlowStateUpdater();

        // Método para enviar datos estáticos (Elevación, Rugosidad, dt, dx) una sola vez
        void initialize(const MapGrid& grid, float dt, StepType steps_batch) override;

        // Solo envía capas dinámicas
        void update() override;

        void render(const MapGrid& grid, Snapshot& snapshot_) override;

    private:
        TF_Graph* graph_;
        TF_Session* session_;
        TF_Status* status_;

        // --- Estructura Auxiliar para Puertos TF ---
        struct TF_Port {
            TF_Output op;
            // Helper para construir desde nombre e índice
            static TF_Port From(TF_Graph* g, const std::string& name, int idx = 0);
        };

        // --- PUERTOS DE INICIALIZACIÓN (Método 'init') ---
        // Inputs
        TF_Port p_init_dt_;
        TF_Port p_init_dx_;
        TF_Port p_init_elevation_;
        TF_Port p_init_roughness_;
        TF_Port p_init_water_depth_;
        TF_Port p_init_rainfall_;
        TF_Port p_init_steps_batch_;
        TF_Port p_init_map_origin_x_;
        TF_Port p_init_map_origin_y_;

        // Outputs
        TF_Port out_init_trigger_;

        // --- PUERTOS DE EJECUCIÓN (Método 'run_batch') ---
        // Outputs
        TF_Port out_run_batch_trigger_;

        // Inputs
        TF_Port p_render_view_min_x_;
        TF_Port p_render_view_max_x_;
        TF_Port p_render_view_min_y_;
        TF_Port p_render_view_max_y_;

        // Outputs
        TF_Port out_render_changed_x_;  // idx 0
        TF_Port out_render_changed_y_;  // idx 1
        TF_Port out_render_water_depth_;// idx 2

        // Helpers de memoria TensorFlow
        template <typename T>
        TF_Tensor* createScalar(T value) = delete;

        template <typename T>
        TF_Tensor* createGridTensor(const T* data, int64_t rows, int64_t cols) = delete;

        void checkStatus(const std::string& context);
    };

    template <>
    TF_Tensor* TensorFlowStateUpdater::createScalar<float>(float value);

    template <>
    TF_Tensor* TensorFlowStateUpdater::createScalar<int64_t>(int64_t value);

    template <>
    TF_Tensor* TensorFlowStateUpdater::createGridTensor<float>(const float* data, int64_t rows, int64_t cols);

    template <>
    TF_Tensor* TensorFlowStateUpdater::createGridTensor<int8_t>(const int8_t* data, int64_t rows, int64_t cols);

} // namespace danasim
