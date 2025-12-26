
#pragma once

#include "core/ports/StateUpdaterPort.hpp"

// === TENSORFLOW C API ===
extern "C" {
    #include "tensorflow/c/c_api.h"
}

namespace danasim {

    struct CachedLayer {
        TF_Output input_op;
        TF_Output output_op; // Solo para capas dinámicas
        bool is_dynamic;
    };

    class TensorFlowStateUpdater final : public StateUpdaterPort {
    public:
        explicit TensorFlowStateUpdater(const std::string& modelPath);
        ~TensorFlowStateUpdater();

        void update(MapGrid& grid, float dt) override;

    private:
        std::vector<CachedLayer> cached_layers_;
        TF_Output dt_op_, dx_op_;

        TF_Output op_changed_x_;
        TF_Output op_changed_y_;

        TF_Graph* graph_;
        TF_Session* session_;
        TF_Status* status_;
    };

} // namespace danasim
