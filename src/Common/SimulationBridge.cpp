
#include "Common/SimulationBridge.h"

SimulationBridge::SimulationBridge(SyncMode mode) : currentMode(mode) {}

void SimulationBridge::pushState(StatePtr newState) {
    std::unique_lock<std::mutex> lock(mtx);

    if (currentMode == SyncMode::RealTime_SkipFrames) {
        // Lógica RealTime: Sobrescribir sin piedad
        latestSnapshot = newState;
        hasNewRealTimeData = true;
        cv_consumer.notify_one(); 
    } else {
        // Lógica Lossless: Bloquear si está lleno
        cv_producer.wait(lock, [this]() {
            return bufferQueue.size() < MAX_QUEUE_SIZE || !isRunning.load();
        });

        if (!isRunning.load()) return;

        bufferQueue.push(newState);
        cv_consumer.notify_one();
    }
}

bool SimulationBridge::waitForState(StatePtr& outState) {
    std::unique_lock<std::mutex> lock(mtx);

    // Esperar mientras no haya datos Y la simulación siga activa
    cv_consumer.wait(lock, [this]() {
        if (!isRunning.load()) return true; // Salir
        
        if (currentMode == SyncMode::RealTime_SkipFrames) return hasNewRealTimeData;
        else return !bufferQueue.empty();
    });

    if (!isRunning.load() && 
        (currentMode == SyncMode::RealTime_SkipFrames ? !hasNewRealTimeData : bufferQueue.empty())) {
        return false; // Fin de simulación
    }

    if (currentMode == SyncMode::RealTime_SkipFrames) {
        outState = latestSnapshot;
        hasNewRealTimeData = false;
    } else {
        outState = bufferQueue.front();
        bufferQueue.pop();
        // Avisar al Core que hay espacio libre
        cv_producer.notify_one();
    }
    return true;
}

void SimulationBridge::stop() {
    isRunning.store(false);
    cv_consumer.notify_all();
    cv_producer.notify_all();
}

bool SimulationBridge::isSimulationRunning() const { return isRunning.load(); }
