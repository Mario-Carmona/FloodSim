
#include "UI/UserInterface.h"

void UserInterface::startApplication() {
    std::cout << "=== DANA SIMULATOR ===" << std::endl;
        
    // 1. Configuration (User Inputs)
    SimConfig cfg;
    userPressStart(cfg);

    std::cout << "Mode: " << (cfg.mode == SyncMode::RealTime_SkipFrames ? "RealTime" : "Lossless") << std::endl;

    // 2. Initialization of components
    bridge = std::make_shared<SimulationBridge>(cfg.mode);
    
    core = std::make_shared<CoreEngineCA>(*bridge);
    core->initialize(cfg);

    std::string outputFolder = ".\\..\\..\\..\\output_gis";
    std::filesystem::create_directories(outputFolder);

    // 3. Launch Core in a secondary thread
    coreThread = std::thread(&CoreEngineCA::runSimulation, core);

    // 4. Enter rendering loop (Main Thread)
    renderLoop();

    // Cleaning
    if(coreThread.joinable()) coreThread.join();
}
