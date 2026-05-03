
#include "core/SimulationCore.hpp"

#include <cmath>
#include <fmt/chrono.h>
#include <algorithm>
#include <GeographicLib/UTMUPS.hpp>

#include "logging/Logger.hpp"
#include "core/snapshot/Snapshot.hpp"
#include "core/common/SimulationConstants.hpp"

namespace danasim {

    SimulationCore::SimulationCore(
        StateUpdaterPort* stateUpdater,
        InputPort* mainInputSource,
        const std::unordered_map<std::string, InputPort*>& layersAlternativeInputSource,
        const std::unordered_map<std::string, std::string>& scalarsConfig,
        std::vector<OutputPort*> outputs,
        SnapshotManager* snapshotManager,
        const SimulationConfig& config,
        const std::string& scenarioName
    )
        : stateUpdater_(stateUpdater)
        , mainInputSource_(mainInputSource)
        , layersAlternativeInputSource_(layersAlternativeInputSource)
        , scalarsConfig_(scalarsConfig)
        , outputs_(outputs)
        , snapshotManager_(snapshotManager)
        , startTimestamp_(config.startTimestamp)
        , timeStep_(config.timeStep)
        , simulationDuration_(config.duration)
        , scenarioName_(scenarioName)
        , view_(config.viewBox)
    {
        // Initialize State
        currentGrid_.load(stateUpdater_->getModelParamsInfo(), mainInputSource_, layersAlternativeInputSource_, scalarsConfig_, timeStep_, startTimestamp_);

        calculateGridView();

        stateUpdater_->initialize(currentGrid_);

        for (auto& output : outputs_) {
            output->setInitConfig(currentGrid_, mainInputSource_->getDatasetName(), startTimestamp_);
        }
    }

    void SimulationCore::run(const std::atomic<bool>& stopFlag)
    {
        LOG_INFO("Simulation started");

        std::chrono::sys_seconds currentTime = startTimestamp_;
        std::chrono::sys_seconds finishTimestamp = currentTime + simulationDuration_;

        currentSnapshot_.set(currentTime, currentGrid_);

        while (currentTime < finishTimestamp) {
            // --- PASO 1: CARGAR DATOS EXTERNOS ---
            // Esto lee del HDF5 y escribe en active_.layers_[RainIntensity]
            currentGrid_.updateDynamicLayers(currentTime);

            auto start = std::chrono::high_resolution_clock::now();

            for (StepType i = 0; i < snapshotManager_->everyNSteps(); ++i) {
                stateUpdater_->step(currentGrid_);

                if (stopFlag.load(std::memory_order_relaxed)) {
                    break; // Salimos del bucle pacíficamente
                }
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            LOG_INFO("TF Graph Execution time: {}s", elapsed.count());
            
            if (stopFlag.load(std::memory_order_relaxed)) {
                LOG_WARN("Simulation halted early by user request.");
                break; // Salimos del bucle pacíficamente
            }

            currentTime += (snapshotManager_->everyNSteps() * timeStep_);

            // --- PASO 3: OUTPUT ---
            publishCurrentState(currentTime);
        }

        LOG_INFO("Simulation finished");
    }

    void SimulationCore::publishCurrentState(std::chrono::sys_seconds time) {
        // 1. Esperar a que los consumidores liberen el buffer anterior
        snapshotManager_->waitForReady();

        calculateViewChanges();

        currentSnapshot_.set(time, currentGrid_);

        // 3. Publicar
        snapshotManager_->publish(&currentSnapshot_, &changes_);

        LOG_INFO("Publish snapshot at time {:%FT%T}", time);
    }

    void SimulationCore::calculateGridView() {
        if (view_.useFullGrid) {
            gridIndexView_.minCol = 0u;
            gridIndexView_.maxCol = currentGrid_.cols();

            gridIndexView_.minRow = 0u;
            gridIndexView_.maxRow = currentGrid_.rows();
        }
        else {
            GridViewBox gridView{
                .southWest = currentGrid_.transformViewPoint(view_.southWest, currentGrid_.crs()),
                .northEast = currentGrid_.transformViewPoint(view_.northEast, currentGrid_.crs())
            };

            gridIndexView_.minCol = static_cast<GridIndexType>(std::floor((gridView.southWest.x - currentGrid_.mapOriginX()) / currentGrid_.cellSize()));
            gridIndexView_.maxCol = static_cast<GridIndexType>(std::ceil((gridView.northEast.x - currentGrid_.mapOriginX()) / currentGrid_.cellSize()));

            gridIndexView_.minCol = std::clamp(gridIndexView_.minCol, static_cast<GridIndexType>(0), currentGrid_.cols());
            gridIndexView_.maxCol = std::clamp(gridIndexView_.maxCol, static_cast<GridIndexType>(0), currentGrid_.cols());

            gridIndexView_.minRow = static_cast<GridIndexType>(std::floor((currentGrid_.mapOriginY() - gridView.northEast.y) / currentGrid_.cellSize()));
            gridIndexView_.maxRow = static_cast<GridIndexType>(std::ceil((currentGrid_.mapOriginY() - gridView.southWest.y) / currentGrid_.cellSize()));

            gridIndexView_.minRow = std::clamp(gridIndexView_.minRow, static_cast<GridIndexType>(0), currentGrid_.rows());
            gridIndexView_.maxRow = std::clamp(gridIndexView_.maxRow, static_cast<GridIndexType>(0), currentGrid_.rows());
        }
    }

    void SimulationCore::calculateViewChanges() {
        // 3. Preparar Hilos
        std::vector<std::thread> threads;

        unsigned int hw_threads = std::thread::hardware_concurrency();
        unsigned int num_threads = (hw_threads > 4) ? hw_threads - 1 : std::max(1u, hw_threads);

        std::vector<ThreadResult> results(num_threads);

        // Calculamos cuántas filas le tocan a cada hilo (del total de filas visibles)
        GridIndexType total_view_rows = gridIndexView_.maxRow - gridIndexView_.minRow;
        GridIndexType rows_per_thread = total_view_rows / num_threads;
        GridIndexType remainder = total_view_rows % num_threads;

        GridIndexType current_start_row = gridIndexView_.minRow;

        // 4. Lanzar Hilos (Fork)
        for (unsigned int i = 0; i < num_threads; ++i) {
            GridIndexType count = rows_per_thread + (i < remainder ? 1 : 0);
            GridIndexType end_row = current_start_row + count;

            if (count > 0) {
                threads.emplace_back(&SimulationCore::processChunk, this,
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
        for (const auto& res : results) total_changes += res.local_indexes.size();

        changes_.indexes.clear();

        changes_.indexes.reserve(total_changes);

        // Copiar datos
        for (const auto& res : results) {
            changes_.indexes.insert(changes_.indexes.end(), res.local_indexes.begin(), res.local_indexes.end());
        }
    }

    void SimulationCore::processChunk(GridIndexType start_row, GridIndexType end_row, ThreadResult& result) {
        // Pre-reservamos memoria local para evitar reallocs constantes
        // Estimamos un 5% de cambios para no desperdiciar RAM
        size_t estimated_changes = static_cast<size_t>((end_row - start_row) * (gridIndexView_.maxCol - gridIndexView_.minCol) * 0.05);
        result.local_indexes.reserve(estimated_changes);

        // --- BUCLE CORREGIDO Y OPTIMIZADO ---
        // Eliminamos el memcpy de arriba y hacemos todo en un pase.
        for (GridIndexType y = start_row; y < end_row; ++y) {
            size_t row_offset = static_cast<size_t>(y * currentGrid_.cols());
            const int8_t* curr_row = currentGrid_.getLayer<int8_t>("flood_risk")->getData().data() + row_offset;
            const int8_t* prev_row = currentSnapshot_.floodRisk().data() + row_offset;

            // Parte 2: DENTRO de la vista (Comparar + Copiar + Detectar)
            for (GridIndexType x = gridIndexView_.minCol; x < gridIndexView_.maxCol; ++x) {
                if (prev_row[x] != curr_row[x]) {
                    result.local_indexes.push_back(y * currentGrid_.cols() + x);
                }
            }
        }
    }

} // namespace danasim
