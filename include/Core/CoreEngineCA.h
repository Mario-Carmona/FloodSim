
#pragma once
#include "Common/Types.h"
#include "Common/SimulationBridge.h"
#include <memory>

// TensorFlow Includes (necesarios aquí porque usamos unique_ptr<Scope> como miembro)
#include "tensorflow/cc/client/client_session.h"
#include "tensorflow/cc/ops/standard_ops.h"
#include "tensorflow/core/framework/tensor.h"

class CoreEngineCA {
private:
    SimulationBridge& bridge;
    SimConfig config;
    StatePtr internalState; // Estado interno mutable

    // TensorFlow pointers
    std::unique_ptr<Scope> rootScope;
    std::unique_ptr<ClientSession> session;
    Output inputStatePH;
    Output evolvedStateOp;

    const float GRAVITY = 9.81f;

    // Construcción del Grafo Computacional (Torricelli + Moore)
    void buildGraph(int rows, int cols);
    // Auxiliar: Packing -> Run -> Unpacking
    void runStepTF();

public:
    explicit CoreEngineCA(SimulationBridge& b);
    void initialize(const SimConfig& cfg, StatePtr initState);
    // Bucle Principal del Hilo de Simulación
    void simulationLoop();
};
