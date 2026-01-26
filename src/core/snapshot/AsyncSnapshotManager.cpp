
#include "core/snapshot/AsyncSnapshotManager.hpp"

#include "app/logging/Logger.hpp"

#include <limits>

namespace danasim {

    AsyncSnapshotManager::AsyncSnapshotManager(const OutputConfig::SnapshotConfig& config, size_t numOutputs)
        : totalOutputs_(numOutputs)
        , remaining_(0)
        , running_(true)
        , everyNSteps_(config.everyNSteps)
        , currentSnapshot_(nullptr)
    {
    }

    AsyncSnapshotManager::~AsyncSnapshotManager() {
        stop();
    }

    void AsyncSnapshotManager::waitForReady() {
        LOG_INFO("Wait for ready");

        std::unique_lock<std::mutex> lock(mutex_);
        // Esperamos si:
        // 1. Todavía hay outputs procesando (remaining > 0)
        // 2. Y el sistema sigue corriendo
        cvCore_.wait(lock, [this] {
            return remaining_ == 0 || !running_;
            });

        LOG_INFO("Wait for ready [DONE]");
    }

    void AsyncSnapshotManager::publish(Snapshot* snapshot) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_) return;

            currentSnapshot_ = snapshot;
            // Reiniciamos la barrera: todos los outputs deben leer esto
            remaining_ = totalOutputs_;
        }
        // Despertamos a TODOS los hilos de salida
        cvOutputs_.notify_all();
    }

    std::pair<Snapshot*, std::unique_ptr<SnapshotReadGuard>> AsyncSnapshotManager::waitForSnapshot(StepType lastStep) {
        std::unique_lock<std::mutex> lock(mutex_);

        cvOutputs_.wait(lock, [this, lastStep] {
            if (!running_) return true; // Salir si paramos
            if (currentSnapshot_ == nullptr) return false; // Esperar si no hay datos

            // Lógica normal: solo avanzamos si hay un paso nuevo
            return currentSnapshot_->step() > lastStep;
            });

        // Si estamos parando, retornamos null
        if (!running_) {
            return { nullptr, nullptr };
        }

        // Creamos el Guard. Cuando el Output lo destruya, llamará a internalSignalDone
        auto guard = std::make_unique<SnapshotReadGuard>([this]() {
            this->internalSignalDone();
            });

        return { currentSnapshot_, std::move(guard) };
    }

    void AsyncSnapshotManager::internalSignalDone() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (remaining_ > 0) {
            remaining_--;
            // Si todos han terminado, despertamos al Core
            if (remaining_ == 0) {
                cvCore_.notify_one();
            }
        }
    }

    void AsyncSnapshotManager::stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            running_ = false;
            // Importante: Resetear remaining para que el Core no se quede bloqueado si estaba esperando
            remaining_ = 0;
        }
        // Despertar a TODO el mundo para que salgan de sus wait()
        cvOutputs_.notify_all();
        cvCore_.notify_all();
    }

} // namespace danasim
