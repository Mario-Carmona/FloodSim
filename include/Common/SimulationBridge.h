
#pragma once
#include "Types.h"
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

//  Gestiona la comunicación Thread-Safe entre Core y UI
class SimulationBridge {
private:
    std::mutex mtx;
    std::condition_variable cv_consumer; // Despierta a la UI
    std::condition_variable cv_producer; // Despierta al Core (solo modo Lossless)

    // Cola para modo Lossless
    std::queue<StatePtr> bufferQueue;
    const size_t MAX_QUEUE_SIZE = 3; // Buffer pequeño para forzar backpressure rápido

    // Puntero único para modo RealTime
    StatePtr latestSnapshot = nullptr;
    bool hasNewRealTimeData = false;

    SyncMode currentMode;
    std::atomic<bool> isRunning {true};

public:
    explicit SimulationBridge(SyncMode mode);
    
    // Métodos principales

    // --- LADO CORE (Productor) ---
    void pushState(StatePtr newState);
    // --- LADO UI (Consumidor) ---
    bool waitForState(StatePtr& outState);
    void stop();
    bool isSimulationRunning() const;
};
