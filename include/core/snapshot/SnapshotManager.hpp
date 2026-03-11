
#pragma once

#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "app/config/Config.hpp"
#include "core/snapshot/Snapshot.hpp"
#include "core/snapshot/ChangeList.hpp"

namespace danasim {

    /**
     * @brief Clase auxiliar para asegurar que signalDone() se llama
     * automáticamente al salir del ámbito (RAII), evitando deadlocks.
     */
    class SnapshotReadGuard {
    public:
        using DoneCallback = std::function<void()>;

        SnapshotReadGuard(DoneCallback callback) : callback_(callback) {}
        // Deshabilitar copia
        SnapshotReadGuard(const SnapshotReadGuard&) = delete;
        SnapshotReadGuard& operator=(const SnapshotReadGuard&) = delete;
        // Permitir movimiento
        SnapshotReadGuard(SnapshotReadGuard&&) = default;

        ~SnapshotReadGuard() {
            if (callback_) callback_();
        }

    private:
        DoneCallback callback_;
    };

    /**
     * @brief Base class for snapshot management.
     */
    class SnapshotManager {
    public:
        SnapshotManager(const OutputConfig::SnapshotConfig& config, size_t numOutputs);
        virtual ~SnapshotManager();

        // Core: Espera a que los consumidores terminen
        virtual void waitForReady();

        // Core: Publica nuevo snapshot
        virtual void publish(const Snapshot* snapshot, const ChangeList* changes);
        
        // Output: Espera datos. Retorna un par {Snapshot, Guard}
        // El Guard llamará a signalDone automáticamente al destruirse.
        std::pair<std::pair<const Snapshot&, const ChangeList&>, std::unique_ptr<SnapshotReadGuard>> waitForSnapshot(std::chrono::system_clock::time_point lastStep);
        
        void stop();
        bool isRunning() const noexcept { return running_; }
        virtual StepType everyNSteps() const noexcept { return everyNSteps_; }

    private:
        // Uso interno para decrementar contador
        void internalSignalDone();

        std::mutex mutex_;
        std::condition_variable cvCore_;    // Core espera aquí
        std::condition_variable cvOutputs_; // Outputs esperan aquí

        const Snapshot* currentSnapshot_; // Snapshot compartido
        const ChangeList* changes_;

        size_t totalOutputs_;
        size_t remaining_; // Cuántos outputs faltan por leer el snapshot actual

        std::atomic<bool> running_;
        StepType everyNSteps_;
    };

} // namespace danasim
