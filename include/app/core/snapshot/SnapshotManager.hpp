/**
 * @file SnapshotManager.hpp
 * @brief Manages the synchronization and distribution of simulation snapshots.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

#include "app/config/Config.hpp"
#include "app/core/snapshot/ChangeList.hpp"
#include "app/core/snapshot/Snapshot.hpp"

namespace floodsim::app::core::snapshot {

/**
 * @class SnapshotReadGuard
 * @brief RAII helper class to ensure done signals are automatically emitted.
 *
 * Prevents deadlocks by guaranteeing that a notification is sent back to the
 * manager when the guard goes out of scope, regardless of how the block exits
 * (e.g., normal return, exception).
 */
class SnapshotReadGuard {
public:
    using DoneCallback = std::function<void()>;

    /**
     * @brief Constructs a read guard with a specified callback.
     *
     * @param callback The function to execute upon destruction.
     */
    SnapshotReadGuard(DoneCallback callback) : callback_(callback) {}
    
    // Disable copy operations
    SnapshotReadGuard(const SnapshotReadGuard&) = delete;
    SnapshotReadGuard& operator=(const SnapshotReadGuard&) = delete;

    // Allow move operations
    SnapshotReadGuard(SnapshotReadGuard&&) = default;
    SnapshotReadGuard& operator=(SnapshotReadGuard&&) = default;

    /**
     * @brief Destructor. Executes the completion callback automatically.
     */
    ~SnapshotReadGuard() {
        if (callback_) callback_();
    }

private:
    DoneCallback callback_;
};

/**
 * @class SnapshotManager
 * @brief Base class for thread-safe simulation snapshot management.
 *
 * Coordinates the handoff of state snapshots between the core simulation engine
 * (producer) and multiple output adapters (consumers) using a thread-safe
 * synchronization protocol.
 */
class SnapshotManager {
public:
    /**
     * @brief Constructs a SnapshotManager.
     *
     * @param config Configuration parameters for snapshot generation.
     * @param num_outputs The total number of output consumers waiting for data.
     */
    SnapshotManager(const config::OutputConfig::SnapshotConfig& config, size_t num_outputs);

    virtual ~SnapshotManager();

    /**
     * @brief Blocks the core engine until all consumers have finished processing the current snapshot.
     */
    virtual void WaitForReady();

    /**
     * @brief Publishes a new snapshot to be consumed by the output adapters.
     *
     * @param snapshot Pointer to the simulation state snapshot.
     * @param changes Pointer to the differential changes list.
     */
    virtual void Publish(const Snapshot* snapshot, const ChangeList* changes);
        
    /**
     * @brief Waits for new snapshot data to become available.
     *
     * Outputs call this method to block until the core publishes a frame newer
     * than the specified timestamp.
     *
     * @param last_step The timestamp of the last processed simulation step.
     * @return std::pair A pair containing the simulation data and an RAII safety guard.
     */
    std::pair<std::pair<const Snapshot&, const ChangeList&>, std::unique_ptr<SnapshotReadGuard>>
        WaitForSnapshot(std::chrono::system_clock::time_point last_step);
        
    /**
     * @brief Signals the manager to stop operations and unblocks waiting threads.
     */
    void Stop();

    /**
     * @brief Checks if the snapshot manager is still actively running.
     *
     * @return true If the manager is active.
     * @return false If the manager has been stopped.
     */
    [[nodiscard]] bool IsRunning() const noexcept { return running_.load(); }

    /**
     * @brief Retrieves the step interval for snapshot generation.
     *
     * @return StepType The number of steps between consecutive snapshots.
     */
    [[nodiscard]] virtual StepType EveryNSteps() const noexcept { return every_n_steps_; }

private:
    /**
     * @brief Internal callback to decrement the active consumer counter.
     */
    void InternalSignalDone();

    std::mutex mutex_;
    std::condition_variable cv_core_;     ///< Core engine waits on this condition.
    std::condition_variable cv_outputs_;  ///< Output consumers wait on this condition.

    const Snapshot* current_snapshot_;    ///< Shared pointer to the current snapshot.
    const ChangeList* changes_;           ///< Shared pointer to the current changes list.

    size_t total_outputs_;
    size_t remaining_;                    ///< Count of outputs that still need to process the current snapshot.

    std::atomic<bool> running_;
    StepType every_n_steps_;
};

} // namespace floodsim::app::core::snapshot
