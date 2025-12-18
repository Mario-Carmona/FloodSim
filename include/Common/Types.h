
#pragma once
#include <vector>
#include <string>
#include <memory>
#include <filesystem>

// Modo de Sincronización entre Core y UI
enum class SyncMode {
    RealTime_SkipFrames,  // Prioridad: Latencia baja. Sobrescribe frames viejos.
    Lossless_AllFrames    // Prioridad: Integridad. Bloquea el Core si la UI es lenta.
};

struct SimConfig {
    std::filesystem::path gisDataPath;
    double cellSize;
    double totalDuration;
    double timeStep;       // dt (segundos)
    int snapshotFreq;      // Cada cuántos pasos se envía un frame a la UI
    SyncMode mode;     // El modo elegido
};

// Matriz auxiliar para manejo de capas en CPU
struct LayerMatrix {
    int rows = 0;
    int cols = 0;
    std::vector<float> data; 

    void resize(int r, int c, float val = 0.0f) {
        rows = r; cols = c;
        data.assign(r * c, val);
    }
    // Acceso seguro (o rápido si quitamos el check)
    float& at(int r, int c) { return data[r * cols + c]; }
    float at(int r, int c) const { return data[r * cols + c]; }
};

// Estado completo del Autómata (Visible para la UI)
struct CellularAutomatonState {
    int rows = 0;
    int cols = 0;
    double timestamp = 0.0;
    int stepIndex = 0;

    // Capas explícitas
    LayerMatrix layerElevation;  // Estática
    LayerMatrix layerRoughness;  // Estática
    LayerMatrix layerCellState;  // Dinámica (Secundaria)
    LayerMatrix layerWaterDepth; // Dinámica (Principal)
};

using StatePtr = std::shared_ptr<CellularAutomatonState>;
