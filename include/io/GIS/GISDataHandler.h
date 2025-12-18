
#pragma once
#include "Common/Types.h"
#include <filesystem>
#include <string>

class GISDataHandler {
public:
    // Carga archivos .rst/.rdc simulada
    static bool loadGISData(const std::filesystem::path& gisDir, CellularAutomatonState& state);

    static void saveLayerToAscii(const std::string& filename, const LayerMatrix& layer, double cellSize);

private:
    static bool loadGISLayer(const std::filesystem::path& layerDir, const std::string& layerName, LayerMatrix& matrix);
    static bool loadRasterWithGDAL(const std::filesystem::path& fullPath, LayerMatrix& matrix);
};
