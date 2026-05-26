/**
 * @file SnapshotManager.cpp
 * @brief Implementation of the SnapshotManager class for thread-safe snapshot distribution.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/core/snapshot/SnapshotManager.hpp"

#include <limits>

#include "logging/Logger.hpp"

namespace floodsim::app::core::snapshot {

using DataPair = std::pair<const Snapshot&, const ChangeList&>;

SnapshotManager::SnapshotManager(const config::OutputConfig::SnapshotConfig& config, size_t num_outputs)
    : total_outputs_(num_outputs)
    , remaining_(0)
    , running_(true)
    , every_n_steps_(config.every_n_steps)
    , current_snapshot_(nullptr)
    , changes_(nullptr) {}

SnapshotManager::~SnapshotManager() {
    Stop();
}

void SnapshotManager::WaitForReady() {
    LOG_INFO("Wait for ready");

    std::unique_lock<std::mutex> lock(mutex_);

    // Wait conditions:
    // 1. All outputs have finished processing (remaining == 0)
    // 2. OR the system has been stopped (!running_)
    cv_core_.wait(lock, [this] {
        return remaining_ == 0 || !running_.load();
        });

    LOG_INFO("Wait for ready [DONE]");
}

void SnapshotManager::Publish(const Snapshot* snapshot, const ChangeList* changes) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_.load()) return;

        current_snapshot_ = snapshot;
        changes_ = changes;

        // Reset the remaining outputs counter to the total expected
        remaining_ = total_outputs_;
    }

    // Notify all waiting output threads that a new snapshot is available
    cv_outputs_.notify_all();
}

std::pair<DataPair, std::unique_ptr<SnapshotReadGuard>> SnapshotManager::WaitForSnapshot(
        std::chrono::system_clock::time_point last_step) {

    std::unique_lock<std::mutex> lock(mutex_);

    cv_outputs_.wait(lock, [this, last_step] {
        // Unblock if stopped
        if (!running_.load()) return true;

        // Wait if there is no data available
        if (current_snapshot_ == nullptr) return false;

        // Standard logic: only unblock if the available step is newer
        return current_snapshot_->Time() > last_step;
        });

    // If we are stopping, return empty safety guards and current data
    if (!running_.load()) {
        return { DataPair(*current_snapshot_, *changes_), nullptr };
    }

    // Create the RAII Guard. When the output adapter destroys it, 
    // it will automatically invoke InternalSignalDone().
    auto guard = std::make_unique<SnapshotReadGuard>([this]() {
        this->InternalSignalDone();
        });

    return { DataPair(*current_snapshot_, *changes_), std::move(guard) };
}

void SnapshotManager::InternalSignalDone() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (remaining_ > 0) {
        remaining_--;

        // Wake up the core thread if all outputs have finished processing
        if (remaining_ == 0) {
            cv_core_.notify_one();
        }
    }
}

void SnapshotManager::Stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;

        // Crucial: Reset remaining count so the core thread does not remain deadlocked
        remaining_ = 0;
    }

    // Wake up all waiting threads (core and outputs) so they can gracefully exit
    cv_core_.notify_all();
    cv_outputs_.notify_all();
}

} // namespace floodsim::app::core::snapshot
