
#include "UI/CLI/CLI_Interface.h"
#include "IO/GIS/GISDataHandler.h"

namespace fs = std::filesystem;

void CLI_Interface::userPressStart(SimConfig& cfg) {
    cfg.gisDataPath = fs::path(".\\..\\..\\..\\data\\initial");
    cfg.totalDuration = 50.0;
    cfg.timeStep = 0.5;
    cfg.snapshotFreq = 2;
    cfg.mode = SyncMode::Lossless_AllFrames;
}

void CLI_Interface::renderLoop() {
    StatePtr currentState = nullptr;
        
    while (true) {
        // Active (or passive, depending on implementation) wait for the next frame
        bool active = bridge->waitForState(currentState);
        
        if (!active) break; // Leave if the bridge closes

        std::string outputFolder = ".\\..\\..\\..\\output_gis";

        GISDataHandler::saveLayerToAscii(outputFolder + "/water_step_" + std::to_string(currentState->stepIndex) + ".asc", currentState->layerWaterDepth, 1.0);
    }
}
