
#pragma once
#include "Common/SimulationBridge.h"
#include "Core/CoreEngineCA.h"
#include <memory>
#include <thread>
#include <iostream>

class UserInterface {
public:
    virtual ~UserInterface() = default;
    
    // Método común: Configura e inicia todo
    void startApplication(); 

protected:
    // Protegidos para que CLI y GUI puedan acceder
    std::shared_ptr<SimulationBridge> bridge;
    std::shared_ptr<CoreEngineCA> core;
    std::thread coreThread;

    virtual void userPressStart(SimConfig& cfg) = 0;

    // Método virtual puro: Cada hijo debe implementar cómo pintar
    virtual void renderLoop() = 0;
};
