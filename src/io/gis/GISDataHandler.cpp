
#include "IO/GIS/GISDataHandler.h"
#include <iostream>
#include <cmath>

bool GISDataHandler::loadGISData(const std::string& path, CellularAutomatonState& state) {
    std::cout << "[DataHandler] Reading GIS files from: " << path << std::endl;
        
    // Simulación: Grid 20x20 con un "valle"
    int H = 20, W = 20;
    state.rows = H;
    state.cols = W;
    state.layerElevation.resize(H, W);
    state.layerWaterDepth.resize(H, W, 0.0f);

    for(int i=0; i<H; ++i) {
        for(int j=0; j<W; ++j) {
            // Generar cuenco
            float dx = (float)(j - W/2);
            float dy = (float)(i - H/2);
            state.layerElevation.at(i, j) = std::sqrt(dx*dx + dy*dy);
        }
    }
    
    // Fuente de agua en el centro
    state.layerWaterDepth.at(H/2, W/2) = 10.0f;

    std::cout << "[DataHandler] Initial state loaded (" << W << "x" << H << ")." << std::endl;
    return true;
}
