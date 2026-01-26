
#pragma once

#include "core/snapshot/SnapshotManager.hpp"

#include <mutex>
#include <condition_variable>
#include <atomic>

#include "app/config/Config.hpp"

namespace danasim {

    class AsyncSnapshotManager final : public SnapshotManager {
    public:
        AsyncSnapshotManager(const OutputConfig::SnapshotConfig& config, size_t numOutputs);
        ~AsyncSnapshotManager(); // Importante para limpieza

        void waitForReady() override;
        void publish(Snapshot* snapshot) override;

        // Retorna el Snapshot y el Guard que gestiona el signalDone
        std::pair<Snapshot*, std::unique_ptr<SnapshotReadGuard>> waitForSnapshot(StepType lastStep) override;

        void stop() override;
        bool isRunning() const noexcept override { return running_; }
        StepType everyNSteps() const noexcept override { return everyNSteps_; }

    private:
        // Uso interno para decrementar contador
        void internalSignalDone();

        std::mutex mutex_;
        std::condition_variable cvCore_;    // Core espera aquí
        std::condition_variable cvOutputs_; // Outputs esperan aquí

        Snapshot* currentSnapshot_; // Snapshot compartido

        size_t totalOutputs_;
        size_t remaining_; // Cuántos outputs faltan por leer el snapshot actual

        std::atomic<bool> running_;
        StepType everyNSteps_;
    };

} // namespace danasim
