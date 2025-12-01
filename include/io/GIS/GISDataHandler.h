
#pragma once
#include "Common/Types.h"
#include <string>

class GISDataHandler {
public:
    // Carga archivos .rst/.rdc simulada
    bool loadGISData(const std::string& path, CellularAutomatonState& state);
};
