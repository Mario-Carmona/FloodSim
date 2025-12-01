
#include "UI/UserInterface.h"
#include "Data/DataHandlerGIS.h"

void UserInterface::startApplication() {
    std::cout << "=== DANA SIMULATOR ===" << std::endl;
        
    // 1. Configuración (Inputs del usuario)
    SimConfig cfg;
    cfg.gisPath = "./data/terrain";
    cfg.totalDuration = 50.0;
    cfg.timeStep = 0.5;
    cfg.snapshotFreq = 2; // Enviar cada 2 pasos
    
    // Selección de modo (Aquí puedes cambiarlo para probar)
    // cfg.syncMode = SyncMode::RealTime_SkipFrames; 
    cfg.syncMode = SyncMode::Lossless_AllFrames;

    std::cout << "Mode: " << (cfg.syncMode == SyncMode::RealTime_SkipFrames ? "RealTime" : "Lossless") << std::endl;

    // 2. Inicialización de componentes
    bridge = std::make_shared<SimulationBridge>(cfg.syncMode);
    
    DataHandlerGIS loader;
    auto initState = std::make_shared<CellularAutomatonState>();
    loader.loadGISData(cfg.gisPath, *initState);

    core = std::make_shared<CoreEngineCA>(*bridge);
    core->initialize(cfg, initState);

    // 3. Lanzar Core en hilo secundario
    coreThread = std::thread(&CoreEngineCA::simulationLoop, core);

    // 4. Entrar en bucle de renderizado (Hilo Principal)
    renderLoop();

    // Limpieza
    if(coreThread.joinable()) coreThread.join();
}
