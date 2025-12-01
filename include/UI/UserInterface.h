
#pragma once
#include "Common/SimulationBridge.h"
#include "Core/CoreEngineCA.h"
#include <memory>
#include <thread>

class UserInterface {
protected:
    // Protegidos para que CLI y GUI puedan acceder
    std::shared_ptr<SimulationBridge> bridge;
    std::shared_ptr<CoreEngineCA> core;
    std::thread coreThread;

    // Método virtual puro: Cada hijo debe implementar cómo pintar
    virtual void renderLoop() = 0; 

public:
    virtual ~UserInterface() = default;
    
    // Método común: Configura e inicia todo
    void startApplication(); 
};
