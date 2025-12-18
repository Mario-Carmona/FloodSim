
#pragma once
#include "Common/Types.h"
#include "Common/SimulationBridge.h"
#include <memory>
#include <vector>

// === TENSORFLOW C API ===
extern "C" {
    #include "tensorflow/c/c_api.h"
}

class CoreEngineCA {
public:
    explicit CoreEngineCA(SimulationBridge& b);
    ~CoreEngineCA();

    // El método initialize ahora incluye la lógica para la generación JIT del ONNX
    void initialize(const SimConfig& cfg);

    void runSimulation();

private:
    void runStepTF();
    TF_Buffer* readBufferFromFile(const char* file); // Helper to load .pb

    SimulationBridge& bridge;
    SimConfig config;
    StatePtr internalState; // Estado interno mutable

    // TensorFlow C Pointers
    TF_Graph* graph = nullptr;
    TF_Session* session = nullptr;
    TF_Status* status = nullptr;

    // Puntos de entrada y salida (Handles)
    TF_Output inputStatePH; // "input_state"
    TF_Output dtPH;         // "dt"
    TF_Output dxPH;         // "dx"
    TF_Output evolvedStateOp; // "output_state"
};
