
#include "adapters/state_updater/TensorFlowStateUpdater.hpp"

#include <fstream>
#include <vector>
#include <cstring>
#include <stdexcept>

#include "app/logging/Logger.hpp"

namespace danasim {

    // Helper estático para buscar operaciones
    TensorFlowStateUpdater::TF_Port TensorFlowStateUpdater::TF_Port::From(TF_Graph* g, const std::string& name, int idx) {
        TF_Operation* op = TF_GraphOperationByName(g, name.c_str());
        if (!op) {
            LOG_ERROR("TF Node not found: {}", name);
            // En un caso real podrías lanzar excepción aquí si es crítico
        }
        return { {op, idx} }; // Construimos el TF_Output interno y el struct TF_Port
    }

    TensorFlowStateUpdater::TensorFlowStateUpdater(const std::filesystem::path& modelPath) {
        status_ = TF_NewStatus();
        graph_ = TF_NewGraph();

        TF_SessionOptions* sess_opts = TF_NewSessionOptions();
        TF_Buffer* run_opts = nullptr;
        const char* tags[] = { "serve" };

        LOG_INFO("Loading TensorFlow SavedModel from: {}", modelPath.string());
        session_ = TF_LoadSessionFromSavedModel(
            sess_opts, run_opts, modelPath.string().c_str(), tags, 1, graph_, nullptr, status_
        );

        if (TF_GetCode(status_) != TF_OK) {
            LOG_ERROR("Failed to load model: {}", TF_Message(status_));
            throw std::runtime_error("Could not load TensorFlow model");
        }
        TF_DeleteSessionOptions(sess_opts);

        // =========================================================
        // MAPEO DE NODOS (Según tu Integration Guide)
        // =========================================================

        // >> METHOD: 'init' (INPUTS)
        p_init_dt_ = TF_Port::From(graph_, "init_dt");
        p_init_dx_ = TF_Port::From(graph_, "init_dx");
        p_init_elevation_ = TF_Port::From(graph_, "init_elevation");
        p_init_roughness_ = TF_Port::From(graph_, "init_roughness");
        p_init_water_depth_ = TF_Port::From(graph_, "init_water_depth");
        p_init_rainfall_ = TF_Port::From(graph_, "init_rainfall");
        p_init_steps_batch_ = TF_Port::From(graph_, "init_steps_batch");
        p_init_map_origin_x_ = TF_Port::From(graph_, "init_map_origin_x");
        p_init_map_origin_y_ = TF_Port::From(graph_, "init_map_origin_y");

        // >> METHOD: 'init' (OUTPUTS)
        out_init_trigger_ = TF_Port::From(graph_, "StatefulPartitionedCall", 0);

        // >> METHOD: 'run_batch' (OUTPUTS)
        out_run_batch_trigger_ = TF_Port::From(graph_, "StatefulPartitionedCall_2", 0);

        // >> METHOD: 'render' (INPUTS)
        p_render_view_min_x_ = TF_Port::From(graph_, "render_view_min_x");
        p_render_view_max_x_ = TF_Port::From(graph_, "render_view_max_x");
        p_render_view_min_y_ = TF_Port::From(graph_, "render_view_min_y");
        p_render_view_max_y_ = TF_Port::From(graph_, "render_view_max_y");

        // >> METHOD: 'render' (OUTPUTS)
        // La tabla dice que todos salen de "StatefulPartitionedCall_1"
        // Indices: 0->state, 1->x, 2->y, 3->water
        std::string render_op_name = "StatefulPartitionedCall_1";

        out_render_changed_x_ = TF_Port::From(graph_, render_op_name, 0);
        out_render_changed_y_ = TF_Port::From(graph_, render_op_name, 1);
        out_render_water_depth_ = TF_Port::From(graph_, render_op_name, 2);
    }

    TensorFlowStateUpdater::~TensorFlowStateUpdater() {
        if (session_) {
            TF_CloseSession(session_, status_);
            TF_DeleteSession(session_, status_);
        }
        if (graph_) TF_DeleteGraph(graph_);
        if (status_) TF_DeleteStatus(status_);
    }

    void TensorFlowStateUpdater::initialize(const MapGrid& grid, float dt, StepType steps_batch) {
        LOG_INFO("Initializing TF Model");

        // 1. Inputs del Init
        std::vector<TF_Output> inputs = {
            p_init_elevation_.op, p_init_roughness_.op,
            p_init_water_depth_.op, p_init_rainfall_.op,
            p_init_dt_.op, p_init_dx_.op,
            p_init_steps_batch_.op,
            p_init_map_origin_x_.op, p_init_map_origin_y_.op
        };

        std::vector<TF_Tensor*> input_values;
        input_values.push_back(createGridTensor<float>(grid.getLayer<float>(LayerId::Elevation).data(), grid.rows(), grid.cols()));
        input_values.push_back(createGridTensor<float>(grid.getLayer<float>(LayerId::Roughness).data(), grid.rows(), grid.cols()));
        input_values.push_back(createGridTensor<float>(grid.getLayer<float>(LayerId::WaterDepth).data(), grid.rows(), grid.cols()));
        input_values.push_back(createGridTensor<float>(grid.getLayer<float>(LayerId::Rainfall).data(), grid.rows(), grid.cols()));
        input_values.push_back(createScalar<float>(dt));
        input_values.push_back(createScalar<float>(grid.cellSize()));
        input_values.push_back(createScalar<StepType>(steps_batch));
        input_values.push_back(createScalar<float>(grid.mapOriginX()));
        input_values.push_back(createScalar<float>(grid.mapOriginY()));

        TF_Tensor* out_tensor = nullptr;

        // 2. Ejecutar Init (Sin outputs relevantes)
        TF_SessionRun(session_, nullptr,
            inputs.data(), input_values.data(), static_cast<int>(inputs.size()),
            &out_init_trigger_.op, &out_tensor, 1,
            nullptr, 0, nullptr, status_
        );

        checkStatus("SessionRun (Initialize)");

        // Limpieza
        for (auto* t : input_values) TF_DeleteTensor(t);
        if (out_tensor) TF_DeleteTensor(out_tensor);
    }

    void TensorFlowStateUpdater::update() {
        // 3. Ejecutar 'run_batch'
        auto start = std::chrono::high_resolution_clock::now();

        TF_Tensor* out_run_batch_tensor = nullptr;

        TF_SessionRun(session_, nullptr,
            nullptr, nullptr, 0,
            &out_run_batch_trigger_.op, &out_run_batch_tensor, 1,
            nullptr, 0, nullptr, status_
        );

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        LOG_INFO("TF Graph Execution time: {}s", elapsed.count());

        checkStatus("SessionRun (Update Batch)");

        // 5. Limpieza
        if (out_run_batch_tensor) TF_DeleteTensor(out_run_batch_tensor);
    }

    void TensorFlowStateUpdater::render(const MapGrid& grid, Snapshot& snapshot_) {
        // 1. Inputs: Estado Actual (State y Water)
        // OJO: El orden en el vector inputs debe coincidir con el vector input_values
        std::vector<TF_Output> inputs = {
            p_render_view_min_x_.op, p_render_view_max_x_.op,
            p_render_view_min_y_.op, p_render_view_max_y_.op
        };

        std::vector<TF_Tensor*> input_values;
        input_values.push_back(createScalar<float>(grid.viewMinX()));
        input_values.push_back(createScalar<float>(grid.viewMaxX()));
        input_values.push_back(createScalar<float>(grid.viewMinY()));
        input_values.push_back(createScalar<float>(grid.viewMaxY()));

        // 2. Outputs Esperados (Ordenados según la tabla y tu código)
        std::vector<TF_Output> outputs = {
            out_render_changed_x_.op,   // 0
            out_render_changed_y_.op,   // 1
            out_render_water_depth_.op  // 2
        };
        std::vector<TF_Tensor*> output_values(outputs.size(), nullptr);

        auto start_render = std::chrono::high_resolution_clock::now();

        TF_SessionRun(session_, nullptr,
            inputs.data(), input_values.data(), static_cast<int>(inputs.size()),
            outputs.data(), output_values.data(), static_cast<int>(outputs.size()),
            nullptr, 0, nullptr, status_
        );

        auto end_render = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_render = end_render - start_render;
        LOG_INFO("Render Execution time: {}s", elapsed_render.count());

        checkStatus("SessionRun (Render)");

        // 4. Extraer Datos de Vuelta a C++

        // Output 2: Water Depth (¡Cuidado con el índice, en tu tabla es el 3!)
        float* water_ptr = static_cast<float*>(TF_TensorData(output_values[2]));

        std::vector<float>& waterSnapshot = snapshot_.waterDepth();
        waterSnapshot.resize(grid.cellCount());

        std::memcpy(waterSnapshot.data(), water_ptr, grid.cellCount() * sizeof(float));

        // Output 0 & 1: Changed Indices
        TF_Tensor* tx = output_values[0];
        TF_Tensor* ty = output_values[1];

        int64_t num_changes = TF_Dim(tx, 0); // Dimensión 0 es el número de elementos

        auto& vec_x = snapshot_.changedX();
        auto& vec_y = snapshot_.changedY();

        vec_x.resize(num_changes);
        vec_y.resize(num_changes);

        if (num_changes > 0) {
            std::memcpy(vec_x.data(), TF_TensorData(tx), num_changes * sizeof(GridIndexType));
            std::memcpy(vec_y.data(), TF_TensorData(ty), num_changes * sizeof(GridIndexType));
        }

        // Limpieza
        for (auto* t : input_values) TF_DeleteTensor(t);
        for (auto* t : output_values) if (t) TF_DeleteTensor(t);
    }

    // --- Helpers de Memoria ---

    template <>
    TF_Tensor* TensorFlowStateUpdater::createScalar<float>(float value) {
        TF_Tensor* t = TF_AllocateTensor(TF_FLOAT, nullptr, 0, sizeof(float));
        std::memcpy(TF_TensorData(t), &value, sizeof(float));
        return t;
    }

    template <>
    TF_Tensor* TensorFlowStateUpdater::createScalar<int64_t>(int64_t value) {
        TF_Tensor* t = TF_AllocateTensor(TF_INT64, nullptr, 0, sizeof(int64_t));
        std::memcpy(TF_TensorData(t), &value, sizeof(int64_t));
        return t;
    }

    template <>
    TF_Tensor* TensorFlowStateUpdater::createGridTensor<float>(const float* data, int64_t rows, int64_t cols) {
        int64_t dims[] = { 1, rows, cols, 1 };
        size_t nbytes = rows * cols * sizeof(float);
        TF_Tensor* t = TF_AllocateTensor(TF_FLOAT, dims, 4, nbytes);
        std::memcpy(TF_TensorData(t), data, nbytes);
        return t;
    }

    template <>
    TF_Tensor* TensorFlowStateUpdater::createGridTensor<int8_t>(const int8_t* data, int64_t rows, int64_t cols) {
        int64_t dims[] = { 1, rows, cols, 1 };
        size_t nbytes = rows * cols * sizeof(int8_t);
        TF_Tensor* t = TF_AllocateTensor(TF_INT8, dims, 4, nbytes);
        std::memcpy(TF_TensorData(t), data, nbytes);
        return t;
    }

    void TensorFlowStateUpdater::checkStatus(const std::string& context) {
        if (TF_GetCode(status_) != TF_OK) {
            LOG_ERROR("TF Error [{}]: {}", context, TF_Message(status_));
            throw std::runtime_error("TensorFlow execution failed");
        }
    }

} // namespace danasim
