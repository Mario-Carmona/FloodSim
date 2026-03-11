
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
        const std::unordered_map<std::string, InputPort*>& layerInputSource,
        std::vector<OutputPort*> outputs,
        SnapshotManager* snapshotManager,
        const SimulationConfig& config,
        const std::string& scenarioName
    )
        : stateUpdater_(stateUpdater)
        , layerInputSource_(layerInputSource)
        , outputs_(outputs)
        , snapshotManager_(snapshotManager)
        , startTimestamp_(config.startTimestamp)
        , timeStep_(config.timeStep)
        , simulationDuration_(config.duration)
        , scenarioName_(scenarioName)
        , currentGrid_(timeStep_)
        , view_(config.viewBox)
    {
        
    }

    void SimulationCore::run()
    {
        LOG_INFO("Simulation started");

        std::chrono::sys_seconds currentTime = startTimestamp_;
        std::chrono::sys_seconds finishTimestamp = currentTime + simulationDuration_;

        // Initialize State
        currentGrid_.load(layerInputSource_, currentTime);

        for (auto& output : outputs_) {
            output->setGrid(currentGrid_);
        }

        calculateGridView();

        stateUpdater_->initialize(currentGrid_, timeStep_);

        // Publicar estado inicial (t=0)
        publishCurrentState(currentTime);

        while (currentTime < finishTimestamp) {
            // --- PASO 1: CARGAR DATOS EXTERNOS ---
            // Esto lee del HDF5 y escribe en active_.layers_[RainIntensity]
            currentGrid_.updateDynamicLayers(currentTime);

            auto start = std::chrono::high_resolution_clock::now();

            for (StepType i = 0; i < snapshotManager_->everyNSteps(); ++i) {
                stateUpdater_->run();
            }

            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> elapsed = end - start;
            LOG_INFO("TF Graph Execution time: {}s", elapsed.count());
            
            currentTime += (snapshotManager_->everyNSteps() * timeStep_);

            // --- PASO 3: OUTPUT ---
            publishCurrentState(currentTime);
        }

        LOG_INFO("Simulation finished");
    }

    void SimulationCore::publishCurrentState(std::chrono::sys_seconds time) {
        // 1. Esperar a que los consumidores liberen el buffer anterior
        snapshotManager_->waitForReady();

        if (time == startTimestamp_) {
            currentSnapshot_.set(time, currentGrid_);
            previousSnapshot_.set(currentSnapshot_.size());
        }
        else {
            std::swap(currentSnapshot_, previousSnapshot_);
            currentSnapshot_.set(time, currentGrid_);
        }

        calculateViewChanges();

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
                .southWest = transformViewPoint(view_.southWest, currentGrid_.crs()),
                .northEast = transformViewPoint(view_.northEast, currentGrid_.crs())
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

    void SimulationCore::processChunk(GridIndexType start_row, GridIndexType end_row, ThreadResult& result) {
        // Pre-reservamos memoria local para evitar reallocs constantes
        // Estimamos un 5% de cambios para no desperdiciar RAM
        size_t estimated_changes = static_cast<size_t>((end_row - start_row) * (gridIndexView_.maxCol - gridIndexView_.minCol) * 0.05);
        result.local_x.reserve(estimated_changes);
        result.local_y.reserve(estimated_changes);

        // --- BUCLE CORREGIDO Y OPTIMIZADO ---
        // Eliminamos el memcpy de arriba y hacemos todo en un pase.
        for (GridIndexType y = start_row; y < end_row; ++y) {
            size_t row_offset = static_cast<size_t>(y * currentGrid_.cols());
            const float* curr_row = currentSnapshot_.data() + row_offset;
            const float* prev_row = previousSnapshot_.data() + row_offset;

            // Parte 2: DENTRO de la vista (Comparar + Copiar + Detectar)
            for (GridIndexType x = gridIndexView_.minCol; x < gridIndexView_.maxCol; ++x) {
                bool now_danger = (curr_row[x] > DANGER_THRESHOLD);
                bool was_danger = (prev_row[x] > DANGER_THRESHOLD);

                if (now_danger != was_danger) {
                    result.local_y.push_back(y);
                    result.local_x.push_back(x);
                }
            }
        }
    }

    std::optional<UTMZoneInfo> SimulationCore::parseUTMZoneFromEPSG(const std::string& epsgString) const {
        // 1. Extraer solo la parte numérica (ignorar "EPSG:" si está presente)
        size_t colonPos = epsgString.find(':');
        std::string numPart = (colonPos != std::string::npos) ?
            epsgString.substr(colonPos + 1) : epsgString;

        int epsgCode;
        try {
            epsgCode = std::stoi(numPart);
        }
        catch (...) {
            return std::nullopt; // No es un número válido
        }

        // 2. Extraer la zona (los dos últimos dígitos)
        int zone = epsgCode % 100;

        // Validación básica de zona UTM
        if (zone < 1 || zone > 60) {
            return std::nullopt;
        }

        // 3. Determinar el sistema y hemisferio
        int baseCode = epsgCode - zone; // Ej: 25830 - 30 = 25800

        switch (baseCode) {
        case 32600: // WGS 84 North
        case 25800: // ETRS89 North (Europa)
        case 23000: // ED50 North (Europa)
            return UTMZoneInfo{ zone, true };

        case 32700: // WGS 84 South
            return UTMZoneInfo{ zone, false };

        default:
            // Es un código EPSG válido numéricamente, pero no es de un bloque UTM reconocido
            return std::nullopt;
        }
    }

    GridViewBox::Point SimulationCore::transformViewPoint(ViewBox::Point sourcePoint, const std::string& targetCRS) const {
        
        auto utmInfo = parseUTMZoneFromEPSG(targetCRS);
        if (!utmInfo.has_value()) {
            throw std::runtime_error("El CRS destino (" + targetCRS + ") no es un sistema UTM soportado matemáticamente.");
        }

        int computedZone;
        bool computedNorthp;

        GridViewBox::Point gridViewPoint;

        try {
            // Le pasamos la zona extraída dinámicamente del string (ej. 30)
            GeographicLib::UTMUPS::Forward(sourcePoint.lat, sourcePoint.lon, computedZone, computedNorthp, gridViewPoint.x, gridViewPoint.y, utmInfo->zone);

            // Verificamos que la coordenada real cae en el hemisferio que el EPSG espera
            if (computedNorthp != utmInfo->isNorth) {
                // Nota: En algunos casos limítrofes (cerca del ecuador) esto podría flexibilizarse,
                // pero estrictamente hablando, si el EPSG pide Norte y cae en Sur, hay una discrepancia.
                throw std::runtime_error("Las coordenadas caen en el hemisferio incorrecto para este EPSG.");
            }

        }
        catch (const GeographicLib::GeographicErr& e) {
            throw std::runtime_error(std::string("Error en GeographicLib: ") + e.what());
        }

        return gridViewPoint;
    }

} // namespace danasim
