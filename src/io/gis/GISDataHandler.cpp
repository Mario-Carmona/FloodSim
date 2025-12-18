
#include "IO/GIS/GISDataHandler.h"
#include <iostream>
#include <cmath>
#include <gdal_priv.h>
#include <fstream>
#include <iomanip>

namespace fs = std::filesystem;

bool GISDataHandler::loadRasterWithGDAL(const fs::path& fullPath, LayerMatrix& matrix) {
    // 1. Open the DataSet (GDAL automatically identifies it as IDRISI)
    GDALDataset* poDataset = (GDALDataset*)GDALOpen(fullPath.string().c_str(), GA_ReadOnly);
    if (poDataset == nullptr) {
        std::cerr << "ERROR: No se pudo abrir el archivo ráster: " << fullPath.string() << std::endl;
        return false;
    }

    // 2. Obtain dimension metadata
    int nXSize = poDataset->GetRasterXSize();
    int nYSize = poDataset->GetRasterYSize();

    if (nXSize <= 0 || nYSize <= 0) {
        std::cerr << "ERROR: Dimensiones ráster inválidas." << std::endl;
        GDALClose(poDataset);
        return false;
    }

    // 3. Initialize the destination array
    matrix.resize(nYSize, nXSize);

    // 4. Obtain the data band
    GDALRasterBand* poBand = poDataset->GetRasterBand(1);
    if (poBand == nullptr) {
        std::cerr << "ERROR: No se pudo obtener la primera banda ráster." << std::endl;
        GDALClose(poDataset);
        return false;
    }

    // 5. Read the data into a float buffer (which matches LayerMatrix)
    // GDAL will adjust the original data type (e.g., Int16) to float.
    CPLErr err = poBand->RasterIO(
        GF_Read, // Read
        0, 0,    // Initial offset (0, 0)
        nXSize, nYSize, // Read block (entire dataset)
        matrix.data.data(), // Pointer to the destination buffer (the LayerMatrix vector)
        nXSize, nYSize, // Buffer size
        GDT_Float32, // Destination buffer data type (float)
        0, 0        // Stride
    );

    // 6. Close and check for errors
    GDALClose(poDataset);

    if (err != CE_None) {
        std::cerr << "ERROR: Error de lectura RasterIO de GDAL." << std::endl;
        return false;
    }

    std::cout << "  > Lectura exitosa: " << nXSize << "x" << nYSize << " píxeles." << std::endl;
    return true;
}

bool GISDataHandler::loadGISLayer(const fs::path& layerDir, const std::string& layerName, LayerMatrix& matrix) {
    fs::path rstPath = layerDir / (layerName + ".rst");

    if (!fs::exists(rstPath)) {
        std::cerr << "ERROR: No se encontró la capa '" << layerName << ".rst' en: " << layerDir.string() << std::endl;
        return false;
    }

    std::cout << "  > Intentando cargar capa: " << layerName << std::endl;
    return loadRasterWithGDAL(rstPath, matrix);
}

bool GISDataHandler::loadGISData(const fs::path& gisDir, CellularAutomatonState& state) {
    GDALAllRegister();

    std::cout << "[DataHandler] Iniciando carga de capas GIS desde: " << gisDir.string() << std::endl;

    // 1. Load Elevation Layer (Required for topology)
    if (!loadGISLayer(gisDir / "DEM_Elevation", "elevation", state.layerElevation)) {
        std::cerr << "FATAL: No se pudo cargar la capa de Elevación." << std::endl;
        return false;
    }

    // 2. Configure the base state (dimensions)
    state.rows = state.layerElevation.rows;
    state.cols = state.layerElevation.cols;

    // 3. Load Initial Water Depth Layer
    // We try to load it, but if it fails, we initialize the layer to zero (fallback behavior).
    if (loadGISLayer(gisDir / "Initial_Water", "initial_water", state.layerWaterDepth)) {
        // We ensure that the dimensions are correct.
        if (state.layerWaterDepth.rows != state.rows || state.layerWaterDepth.cols != state.cols) {
            std::cerr << "ADVERTENCIA: La capa 'initial_water' tiene dimensiones incorrectas. Inicializando a cero." << std::endl;
            state.layerWaterDepth.resize(state.rows, state.cols, 0.0f);
        }
    }
    else {
        std::cout << "ADVERTENCIA: Capa 'initial_water' no encontrada. Inicializando a cero." << std::endl;
        state.layerWaterDepth.resize(state.rows, state.cols, 0.0f);
    }

    if (loadGISLayer(gisDir / "Roughness", "roughness", state.layerRoughness)) {
        // We ensure that the dimensions are correct.
        if (state.layerRoughness.rows != state.rows || state.layerRoughness.cols != state.cols) {
            std::cerr << "ADVERTENCIA: La capa 'roughness' tiene dimensiones incorrectas. Inicializando a cero." << std::endl;
            state.layerRoughness.resize(state.rows, state.cols, 0.0f);
        }
    }
    else {
        std::cout << "ADVERTENCIA: Capa 'roughness' no encontrada. Inicializando a cero." << std::endl;
        state.layerRoughness.resize(state.rows, state.cols, 0.0f);
    }

    std::cout << "ADVERTENCIA: Capa 'roughness' no encontrada. Inicializando a Dynamic." << std::endl;
    state.layerCellState.resize(state.rows, state.cols, 2.0f);

    std::cout << "[DataHandler] Todas las capas necesarias cargadas con éxito (" << state.cols << "x" << state.rows << ")." << std::endl;
    return true;
}

void GISDataHandler::saveLayerToAscii(const std::string& filename, const LayerMatrix& layer, double cellSize) {
    std::ofstream ofs(filename);
    if (!ofs.is_open()) return;

    // Cabecera del formato ESRI ASCII
    ofs << "ncols         " << layer.cols << "\n";
    ofs << "nrows         " << layer.rows << "\n";
    ofs << "xllcorner     0.0\n"; // Coordenadas locales
    ofs << "yllcorner     0.0\n";
    ofs << "cellsize      " << cellSize << "\n";
    ofs << "NODATA_value  -9999\n";

    // Datos de la matriz
    for (int i = 0; i < layer.rows; ++i) {
        for (int j = 0; j < layer.cols; ++j) {
            float val = layer.at(i, j);
            // Si el valor es casi cero, podemos tratarlo como NODATA o 0.0
            if (val < 1e-5f) ofs << "0.0 ";
            else ofs << std::fixed << std::setprecision(4) << val << " ";
        }
        ofs << "\n";
    }
    ofs.close();
}
