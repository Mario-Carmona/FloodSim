
#include "Core/CoreEngineCA.h"
#include "IO/GIS/GISDataHandler.h"
#include <iostream>
#include <fstream>


// Constantes de estado alineadas con el modelo Python
const float ST_EMPTY = 0.0f;
const float ST_STATIC = 1.0f;
const float ST_DYNAMIC = 2.0f;

CoreEngineCA::CoreEngineCA(SimulationBridge& b)
    : bridge(b), internalState(std::make_shared<CellularAutomatonState>()) {
    status = TF_NewStatus();
}

CoreEngineCA::~CoreEngineCA() {
    if (session) TF_DeleteSession(session, status);
    if (graph) TF_DeleteGraph(graph);
    if (status) TF_DeleteStatus(status);
}

void CoreEngineCA::initialize(const SimConfig& cfg) {
    config = cfg;


    GISDataHandler::loadGISData(config.gisDataPath, *internalState);


    graph = TF_NewGraph();

    // 1. Cargar el Grafo Congelado (.pb)
    TF_Buffer* graph_def = readBufferFromFile(".\\..\\..\\..\\models\\model_ca.pb");
    if (!graph_def) {
        std::cerr << "[TF] Error: No se encontró el archivo del modelo." << std::endl;
        return;
    }

    TF_ImportGraphDefOptions* opts = TF_NewImportGraphDefOptions();
    TF_GraphImportGraphDef(graph, graph_def, opts, status);
    TF_DeleteImportGraphDefOptions(opts);
    TF_DeleteBuffer(graph_def);

    if (TF_GetCode(status) != TF_OK) {
        std::cerr << "[TF] Error al importar: " << TF_Message(status) << std::endl;
        return;
    }

    // 2. Vincular los nodos del grafo por nombre
    inputStatePH = { TF_GraphOperationByName(graph, "input_state"), 0 };
    dtPH = { TF_GraphOperationByName(graph, "dt"), 0 };
    dxPH = { TF_GraphOperationByName(graph, "dx"), 0 };
    evolvedStateOp = { TF_GraphOperationByName(graph, "output_state"), 0 };

    // 3. Crear Sesión
    TF_SessionOptions* sess_opts = TF_NewSessionOptions();
    session = TF_NewSession(graph, sess_opts, status);
    TF_DeleteSessionOptions(sess_opts);

    std::cout << "[Core] Motor TensorFlow inicializado correctamente." << std::endl;
}

void CoreEngineCA::runStepTF() {
    const int H = internalState->rows;
    const int W = internalState->cols;

    // --- FASE 1: Empaquetado de Datos (Grid -> Tensor [1, H, W, 4]) ---
    int64_t dims[] = { 1, H, W, 4 };
    size_t data_size = sizeof(float) * H * W * 4;
    std::vector<float> buffer(H * W * 4);

    for (int i = 0; i < H; ++i) {
        for (int j = 0; j < W; ++j) {
            int idx = (i * W + j) * 4;
            buffer[idx + 0] = internalState->layerElevation.at(i, j);
            buffer[idx + 1] = internalState->layerWaterDepth.at(i, j);
            buffer[idx + 2] = internalState->layerRoughness.at(i, j);
            buffer[idx + 3] = (float)internalState->layerCellState.at(i, j);
        }
    }

    TF_Tensor* state_tensor = TF_AllocateTensor(TF_FLOAT, dims, 4, data_size);
    std::memcpy(TF_TensorData(state_tensor), buffer.data(), data_size);

    // --- FASE 2: Tensores Escalares (dt, dx) ---
    TF_Tensor* dt_tensor = TF_AllocateTensor(TF_FLOAT, nullptr, 0, sizeof(float));
    TF_Tensor* dx_tensor = TF_AllocateTensor(TF_FLOAT, nullptr, 0, sizeof(float));

    *(float*)TF_TensorData(dt_tensor) = (float)config.timeStep;
    *(float*)TF_TensorData(dx_tensor) = (float)config.cellSize;

    // --- FASE 3: Ejecución ---
    TF_Output inputs[] = { inputStatePH, dtPH, dxPH };
    TF_Tensor* input_values[] = { state_tensor, dt_tensor, dx_tensor };
    TF_Tensor* output_values[1] = { nullptr };

    TF_SessionRun(session, nullptr,
        inputs, input_values, 3,
        &evolvedStateOp, output_values, 1,
        nullptr, 0, nullptr, status);

    if (TF_GetCode(status) != TF_OK) {
        std::cerr << "[TF] Error en ejecución: " << TF_Message(status) << std::endl;
    }
    else {
        // --- FASE 4: Desempaquetado (Tensor -> Grid) ---
        float* out_ptr = (float*)TF_TensorData(output_values[0]);
        for (int i = 0; i < H; ++i) {
            for (int j = 0; j < W; ++j) {
                int idx = (i * W + j) * 4;
                // Actualizamos profundidad y el nuevo estado calculado por la GPU
                internalState->layerWaterDepth.at(i, j) = out_ptr[idx + 1];
                internalState->layerCellState.at(i, j) = (int)out_ptr[idx + 3];
            }
        }
        TF_DeleteTensor(output_values[0]);
    }

    // Limpieza de tensores de entrada
    TF_DeleteTensor(state_tensor);
    TF_DeleteTensor(dt_tensor);
    TF_DeleteTensor(dx_tensor);
}

void CoreEngineCA::runSimulation() {
    float currentTime = 0;
    int step = 0;

    // =========================================================
    // ENVIAR SNAPSHOT INICIAL (t=0)
    // =========================================================
    {
        auto snapshot = std::make_shared<CellularAutomatonState>(*internalState);
        bridge.pushState(snapshot);
    }
    // =========================================================

    while (currentTime < config.totalDuration) {
        runStepTF();

        currentTime += config.timeStep;
        step++;
        internalState->stepIndex = step;

        // Publicar estado a la UI cada N pasos
        if (step % config.snapshotFreq == 0) {
            auto snapshot = std::make_shared<CellularAutomatonState>(*internalState);
            bridge.pushState(snapshot);
        }

        std::cout << "[Core] Step " << step << ": Completed" << std::endl;
        std::cout << "[Core] Time " << currentTime << " of " << config.totalDuration << std::endl;
    }

    // =========================================================
    // ENVIAR EL SNAPSHOT FINAL (t = End)
    // =========================================================
    {
        auto snapshot = std::make_shared<CellularAutomatonState>(*internalState);
        bridge.pushState(snapshot);
    }
    // =========================================================

    bridge.stop();
}

TF_Buffer* CoreEngineCA::readBufferFromFile(const char* file) {
    std::ifstream ifs(file, std::ios::binary | std::ios::ate);
    if (!ifs.is_open()) return nullptr;
    std::streamsize size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    void* data = std::malloc(size);
    ifs.read(static_cast<char*>(data), size);
    TF_Buffer* buf = TF_NewBuffer();
    buf->data = data;
    buf->length = size;
    buf->data_deallocator = [](void* d, size_t l) { std::free(d); };
    return buf;
}
