
#include "core/simulation/SimulationCore.hpp"

#include <cmath>

#include "app/logging/Logger.hpp"
#include "core/snapshot/Snapshot.hpp"
#include "core/common/SimulationConstants.hpp"

namespace danasim {

    SimulationCore::SimulationCore(
        StateUpdaterPort* stateUpdater,
        InputPort* input,
        std::vector<OutputPort*> outputs,
        SnapshotManager* snapshotManager,
        const SimulationConfig& config,
        const std::string& scenarioName
    )
        : stateUpdater_(stateUpdater)
        , input_(input)
        , outputs_(outputs)
        , snapshotManager_(snapshotManager)
        , timeStep_(config.timeStep)
        , totalTime_(config.totalTime)
        , scenarioName_(scenarioName)
        , view_({ .minX = config.viewMinX, .maxX = config.viewMaxX, .minY = config.viewMinY, .maxY = config.viewMaxY })
    {
        maxSteps_ = static_cast<StepType>(std::ceil(totalTime_ / timeStep_));
    }

    void SimulationCore::run()
    {
        LOG_INFO("Simulation started");
        // Initialize State
        input_->load(currentGrid_, streamedLayerManager_, timeStep_);

        for (auto& output : outputs_) {
            output->setGrid(currentGrid_);
        }

        calculateViewGrid();

        stateUpdater_->initialize(currentGrid_, timeStep_);

        StepType step = 0;

        // Publicar estado inicial (t=0)
        publishCurrentState(step);

        while (step < maxSteps_) {
            // --- PASO 1: CARGAR DATOS EXTERNOS ---
            // Esto lee del HDF5 y escribe en active_.layers_[RainIntensity]
            streamedLayerManager_.updateAllLayers(currentGrid_, step, timeStep_);

            auto start = std::chrono::high_resolution_clock::now();

            for (StepType i = 0; i < snapshotManager_->everyNSteps(); ++i) {
                stateUpdater_->run();
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            LOG_INFO("TF Graph Execution time: {}s", elapsed.count());
            
            step += snapshotManager_->everyNSteps();

            // --- PASO 3: OUTPUT ---
            publishCurrentState(step);
        }

        // Caso borde: Si el último paso no coincidió con snapshotManager_->everyNSteps(), forzamos publicación final
        if (step % snapshotManager_->everyNSteps() != 0) {
            publishCurrentState(step);
        }

        LOG_INFO("Simulation finished");
    }

    void SimulationCore::publishCurrentState(StepType s) {
        // 1. Esperar a que los consumidores liberen el buffer anterior
        snapshotManager_->waitForReady();

        if (s == 0) {
            currentSnapshot_.set(s, s * timeStep_, currentGrid_);
            previousSnapshot_.set(currentSnapshot_.size());
        }
        else {
            std::swap(currentSnapshot_, previousSnapshot_);
            currentSnapshot_.set(s, s * timeStep_, currentGrid_);
        }

        calculateViewChanges();

        // 3. Publicar
        snapshotManager_->publish(&currentSnapshot_, &changes_);

        LOG_INFO("Publish snapshot at step {}", s);
    }

    void SimulationCore::calculateViewGrid() {
        viewGridIndex_.minX = static_cast<GridIndexType>(std::floor((view_.minX - currentGrid_.mapOriginX()) / currentGrid_.cellSize()));
        viewGridIndex_.maxX = static_cast<GridIndexType>(std::ceil((view_.maxX - currentGrid_.mapOriginX()) / currentGrid_.cellSize()));

        viewGridIndex_.minY = static_cast<GridIndexType>(std::floor((currentGrid_.mapOriginY() - view_.maxY) / currentGrid_.cellSize()));
        viewGridIndex_.maxY = static_cast<GridIndexType>(std::ceil((currentGrid_.mapOriginY() - view_.minY) / currentGrid_.cellSize()));
    }

    void SimulationCore::calculateViewChanges() {
        // 3. Preparar Hilos
        std::vector<std::thread> threads;

        unsigned int hw_threads = std::thread::hardware_concurrency();
        unsigned int num_threads = (hw_threads > 4) ? hw_threads - 1 : std::max(1u, hw_threads);

        std::vector<ThreadResult> results(num_threads);

        // Calculamos cuántas filas le tocan a cada hilo (del total de filas visibles)
        int total_view_rows = viewGridIndex_.maxY - viewGridIndex_.minY;
        int rows_per_thread = total_view_rows / num_threads;
        int remainder = total_view_rows % num_threads;

        GridIndexType current_start_row = viewGridIndex_.minY;

        // 4. Lanzar Hilos (Fork)
        for (unsigned int i = 0; i < num_threads; ++i) {
            int count = rows_per_thread + (i < remainder ? 1 : 0);
            GridIndexType end_row = current_start_row + count;

            if (count > 0) {
                threads.emplace_back(&SimulationCore::process_chunk, this,
                    i,
                    current_start_row,
                    end_row,
                    std::ref(results[i])
                );
            }
            current_start_row = end_row;
        }

        // 5. Esperar Hilos (Join)
        for (auto& t : threads) {
            if (t.joinable()) t.join();
        }

        // 6. Fusionar Resultados (Reduce)

        // Calcular tamaño total primero para hacer un solo reserve
        size_t total_changes = 0;
        for (const auto& res : results) total_changes += res.local_x.size();

        changes_.x.clear();
        changes_.y.clear();

        changes_.x.reserve(total_changes);
        changes_.y.reserve(total_changes);

        // Copiar datos
        for (const auto& res : results) {
            changes_.x.insert(changes_.x.end(), res.local_x.begin(), res.local_x.end());
            changes_.y.insert(changes_.y.end(), res.local_y.begin(), res.local_y.end());
        }
    }

    void SimulationCore::process_chunk(int thread_id, GridIndexType start_row, GridIndexType end_row, ThreadResult& result) {
        // Pre-reservamos memoria local para evitar reallocs constantes
        // Estimamos un 5% de cambios para no desperdiciar RAM
        size_t estimated_changes = (end_row - start_row) * (viewGridIndex_.maxX - viewGridIndex_.minX) * 0.05;
        result.local_x.reserve(estimated_changes);
        result.local_y.reserve(estimated_changes);

        // --- BUCLE CORREGIDO Y OPTIMIZADO ---
        // Eliminamos el memcpy de arriba y hacemos todo en un pase.
        for (GridIndexType y = start_row; y < end_row; ++y) {
            size_t row_offset = static_cast<size_t>(y * currentGrid_.cols());
            const float* curr_row = currentSnapshot_.data() + row_offset;
            const float* prev_row = previousSnapshot_.data() + row_offset;

            // Parte 2: DENTRO de la vista (Comparar + Copiar + Detectar)
            for (GridIndexType x = viewGridIndex_.minX; x < viewGridIndex_.maxX; ++x) {
                bool now_danger = (curr_row[x] > DANGER_THRESHOLD);
                bool was_danger = (prev_row[x] > DANGER_THRESHOLD);

                if (now_danger != was_danger) {
                    result.local_y.push_back(y);
                    result.local_x.push_back(x);
                }
            }
        }
    }

} // namespace danasim
