
#include "Core/CoreEngineCA.h"
#include <iostream>
#include <vector>

using namespace tensorflow;
using namespace tensorflow::ops;

CoreEngineCA::CoreEngineCA(SimulationBridge& b) : bridge(b) {}

void CoreEngineCA::initialize(const SimConfig& cfg, StatePtr initState) {
    config = cfg;
    // Clonamos el estado inicial para uso interno
    internalState = std::make_shared<CellularAutomatonState>(*initState);
    
    // Setup TF
    rootScope = std::make_unique<Scope>(Scope::NewRootScope());
    session = std::make_unique<ClientSession>(*rootScope);
    
    buildGraph(initState->rows, initState->cols);
}

void CoreEngineCA::buildGraph(int rows, int cols) {
    // Input: [1, H, W, 2] -> Canal 0: Elev, Canal 1: Agua
    inputStatePH = Placeholder(*rootScope, DT_FLOAT, Placeholder::Attrs().Shape({1, rows, cols, 2}));
    
    auto elev = Slice(*rootScope, inputStatePH, {0,0,0,0}, {1, rows, cols, 1});
    auto water = Slice(*rootScope, inputStatePH, {0,0,0,1}, {1, rows, cols, 1});
    auto head = Add(*rootScope, elev, water);

    std::vector<Output> flows_out;
    int shifts_y[] = {-1, -1, -1,  0, 0,  1, 1, 1};
    int shifts_x[] = {-1,  0,  1, -1, 1, -1, 0, 1};
    float dists[]  = {1.414, 1.0, 1.414, 1.0, 1.0, 1.414, 1.0, 1.414}; 

    auto zero = Const(*rootScope, 0.0f);
    auto dt = Const(*rootScope, (float)config.timeStep);
    auto two_g = Const(*rootScope, 2.0f * GRAVITY);

    for(int k=0; k<8; ++k) {
        // 1. Shift para alinear vecino
        auto neighbor_head = Roll(*rootScope, head, {shifts_y[k], shifts_x[k]}, {1, 2});
        
        // 2. Diff y Gradiente
        auto diff = Sub(*rootScope, head, neighbor_head);
        auto gradient = Maximum(*rootScope, diff, zero); // ReLU
        
        // 3. Torricelli: v = sqrt(2gh)
        auto vel = Sqrt(*rootScope, Multiply(*rootScope, two_g, gradient));
        
        // 4. Flujo Potencial
        auto flow = Multiply(*rootScope, vel, water);
        flow = Multiply(*rootScope, flow, Const(*rootScope, (float)config.timeStep / dists[k]));
        flow = Multiply(*rootScope, flow, Const(*rootScope, 0.1f)); // Fricción / Estabilidad
        
        flows_out.push_back(flow);
    }

    // 5. Balance de Masas (Limiter)
    auto sum_out = AddN(*rootScope, flows_out);
    auto safe_water = Maximum(*rootScope, water, Const(*rootScope, 1e-5f));
    auto fraction = Div(*rootScope, safe_water, Add(*rootScope, sum_out, safe_water));
    auto limiter = Minimum(*rootScope, fraction, Const(*rootScope, 1.0f));

    // 6. Calcular Flujos Netos
    std::vector<Output> net_fluxes;
    for(int k=0; k<8; ++k) {
        auto out_f = Multiply(*rootScope, flows_out[k], limiter);
        // Inflow es el Outflow del vecino en dirección opuesta
        auto in_f = Roll(*rootScope, out_f, {-shifts_y[k], -shifts_x[k]}, {1, 2});
        net_fluxes.push_back(Sub(*rootScope, in_f, out_f));
    }

    auto net_change = AddN(*rootScope, net_fluxes);
    auto new_water = Add(*rootScope, water, net_change);
    new_water = Maximum(*rootScope, new_water, zero); // Clamp 0

    evolvedStateOp = Concat(*rootScope, 3, {elev, new_water});
}

void CoreEngineCA::simulationLoop() {
    std::cout << "[Core] Simulation thread started." << std::endl;
        
    double currentTime = 0.0;
    int step = 0;

    // =========================================================
    // ENVIAR SNAPSHOT INICIAL (t=0)
    // =========================================================
    {
        // Creamos copia del estado inicial cargado
        auto initialSnapshot = std::make_shared<CellularAutomatonState>(*internalState);
        initialSnapshot->timestamp = 0.0;
        initialSnapshot->stepIndex = 0;

        // Enviamos al bridge.
        // - Si es RealTime: Lo envía y sigue inmediatamente.
        // - Si es Lossless: SE BLOQUEA AQUÍ hasta que la UI lo procese.
        bridge.pushState(initialSnapshot);
    }
    // =========================================================

    // Ahora comienza el bucle de evolución
    while (bridge.isSimulationRunning() && currentTime < config.totalDuration) {
        // 1. Ejecutar TF (Run Step)
        runStepTF();
        
        currentTime += config.timeStep;
        step++;

        internalState->timestamp = currentTime;
        internalState->stepIndex = step;

        // 2. Publicar Snapshot
        if (step % config.snapshotFreq == 0) {
            // COPIA PROFUNDA necesaria para evitar race conditions
            auto snapshot = std::make_shared<CellularAutomatonState>(*internalState);
            
            // Enviamos al bridge (bloqueará o no según el modo)
            bridge.pushState(snapshot);
        }
    }

    // =========================================================
    // ENVIAR EL SNAPSHOT FINAL (t = End)
    // =========================================================
    std::cout << "[Core] Loop finished. Sending Final Snapshot..." << std::endl;
    {
        // 1. Crear copia del estado final exacto
        auto finalSnapshot = std::make_shared<CellularAutomatonState>(*internalState);
        
        // 2. Enviar a la UI.
        // Si estamos en modo Lossless, esta línea BLOQUEARÁ al Core
        // hasta que la UI haya procesado este último frame.
        // Esto garantiza que el vídeo/visualización llegue hasta el final.
        bridge.pushState(finalSnapshot);
    }
    // =========================================================

    std::cout << "[Core] Stopping bridge (Signal Finish)." << std::endl;
    
    // 3. Ahora es seguro cerrar el puente.
    // La UI recibirá 'false' en waitForState SOLO DESPUÉS de haber leído el finalSnapshot.
    bridge.stop();
}

void CoreEngineCA::runStepTF() {
    int H = internalState->rows;
    int W = internalState->cols;

    // Packing
    Tensor inputTensor(DT_FLOAT, TensorShape({1, H, W, 2}));
    auto map = inputTensor.tensor<float, 4>();
    for(int i=0; i<H; ++i) {
        for(int j=0; j<W; ++j) {
            map(0, i, j, 0) = internalState->layerElevation.at(i, j);
            map(0, i, j, 1) = internalState->layerWaterDepth.at(i, j);
        }
    }

    // Run
    std::vector<Tensor> outputs;
    Status s = session->Run({{inputStatePH, inputTensor}}, {evolvedStateOp}, {}, &outputs);
    if(!s.ok()) {
        std::cerr << "TF Error: " << s.ToString() << std::endl;
        bridge.stop();
        return;
    }

    // Unpacking
    auto outMap = outputs[0].tensor<float, 4>();
    for(int i=0; i<H; ++i) {
        for(int j=0; j<W; ++j) {
            internalState->layerWaterDepth.at(i, j) = outMap(0, i, j, 1);
        }
    }
}
