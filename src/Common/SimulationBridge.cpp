
#include "Common/SimulationBridge.h"

SimulationBridge::SimulationBridge(SyncMode mode) : currentMode(mode) {}

void SimulationBridge::pushState(StatePtr newState) {
    std::unique_lock<std::mutex> lock(mtx);

    if (currentMode == SyncMode::RealTime_SkipFrames) {
        // RealTime Logic: Always overwrite
        latestSnapshot = newState;
        hasNewRealTimeData = true;
        cv_consumer.notify_one(); 
    } else {
        // Lossless Logic: Block if full
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

    // Wait until there is no data and the simulation is still active
    cv_consumer.wait(lock, [this]() {
        if (!isRunning.load()) return true; // Exit
        
        if (currentMode == SyncMode::RealTime_SkipFrames) return hasNewRealTimeData;
        else return !bufferQueue.empty();
    });

    if (!isRunning.load() && 
        (currentMode == SyncMode::RealTime_SkipFrames ? !hasNewRealTimeData : bufferQueue.empty())) {
        return false; // End of simulation
    }

    if (currentMode == SyncMode::RealTime_SkipFrames) {
        outState = latestSnapshot;
        hasNewRealTimeData = false;
    } else {
        outState = bufferQueue.front();
        bufferQueue.pop();
        // Notify the Core that there is free space
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
