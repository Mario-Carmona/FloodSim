
#pragma once

#include <memory>
#include <vector>

#include "core/snapshot/Snapshot.hpp"

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
        virtual ~SnapshotManager() = default;

        // Core: Espera a que los consumidores terminen
        virtual void waitForReady() = 0;

        // Core: Publica nuevo snapshot
        virtual void publish(Snapshot* snapshot) = 0;
        
        // Output: Espera datos. Retorna un par {Snapshot, Guard}
        // El Guard llamará a signalDone automáticamente al destruirse.
        virtual std::pair<Snapshot*, std::unique_ptr<SnapshotReadGuard>> waitForSnapshot(StepType lastStep) = 0;
        
        virtual void stop() = 0;
        virtual bool isRunning() const noexcept = 0;
        virtual StepType everyNSteps() const noexcept = 0;
    };

} // namespace danasim
