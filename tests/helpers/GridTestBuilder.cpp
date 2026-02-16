
#pragma once

#include "helpers/GridTestBuilder.hpp"

namespace danasim {

    void GridTestBuilder::createLakeAtRest(MapGrid& grid, GridIndexType rows, GridIndexType cols, float waterDepth) {
        grid.init(rows, cols, 1.0f, 0.0f, 0.0f);
            
        std::vector<float>& waterData = grid.getLayer<float>(LayerId::WaterDepth);
        std::vector<float>& elevationData = grid.getLayer<float>(LayerId::Elevation);

        for (FlatVectorIndexType i = 0; i < grid.cellCount(); ++i) {
            GridIndexType r = i / cols;
            GridIndexType c = i % cols;

            // Si es una celda del borde, construimos un muro de 5 metros por encima del agua
            if (r == 0 || r == rows - 1 || c == 0 || c == cols - 1) {
                elevationData[i] = waterDepth + 5.0f;
                waterData[i] = 0.0f; // Muro seco
            }
            else {
                elevationData[i] = 0.0f; // Fondo plano
                waterData[i] = waterDepth;
            }
        }
    }

    void GridTestBuilder::createIrregularLakeAtRest(MapGrid& grid, GridIndexType rows, GridIndexType cols, float surfaceElevation) {
        grid.init(rows, cols, 1.0f, 0.0f, 0.0f);

        std::vector<float>& waterData = grid.getLayer<float>(LayerId::WaterDepth);
        std::vector<float>& elevationData = grid.getLayer<float>(LayerId::Elevation);

        for (FlatVectorIndexType i = 0; i < grid.cellCount(); ++i) {
            GridIndexType r = i / cols;
            GridIndexType c = i % cols;

            // Muros en los bordes
            if (r == 0 || r == rows - 1 || c == 0 || c == cols - 1) {
                elevationData[i] = surfaceElevation + 5.0f;
                waterData[i] = 0.0f;
            }
            else {
                // Terreno ondulado interior
                float terrainZ = 2.0f + std::sin(r * 0.5f) + std::cos(c * 0.5f);

                if (terrainZ > surfaceElevation - 0.5f) {
                    terrainZ = surfaceElevation - 0.5f;
                }

                elevationData[i] = terrainZ;
                waterData[i] = surfaceElevation - terrainZ; // Z + H = Constante
            }
        }
    }
    
    void GridTestBuilder::createDamBreakInPool(MapGrid& grid, GridIndexType rows, GridIndexType cols) {
        grid.init(rows, cols, 1.0f, 0.0f, 0.0f); // dx = 1.0m

        std::vector<float>& waterData = grid.getLayer<float>(LayerId::WaterDepth);
        std::vector<float>& elevationData = grid.getLayer<float>(LayerId::Elevation);

        for (FlatVectorIndexType i = 0; i < grid.cellCount(); ++i) {
            GridIndexType r = i / cols;
            GridIndexType c = i % cols;

            // Muros en los bordes para que el agua no se escape del modelo
            if (r == 0 || r == rows - 1 || c == 0 || c == cols - 1) {
                elevationData[i] = 100.0f; // Muro alto
                waterData[i] = 0.0f;
            }
            else {
                elevationData[i] = 0.0f; // Suelo interior plano

                // Colocamos un "bloque" de 5 metros de agua justo en el centro (ej. de 4x4 celdas)
                if (r >= rows / 2 - 2 && r <= rows / 2 + 1 && c >= cols / 2 - 2 && c <= cols / 2 + 1) {
                    waterData[i] = 5.0f;
                }
                else {
                    waterData[i] = 0.0f; // El resto del suelo está seco
                }
            }
        }
    }

    void GridTestBuilder::createDropInCenter(MapGrid& grid, GridIndexType size) {
        // size debe ser impar (ej. 31x31) para tener un centro exacto
        grid.init(size, size, 1.0f, 0.0f, 0.0f);

        std::vector<float>& waterData = grid.getLayer<float>(LayerId::WaterDepth);
        std::vector<float>& elevationData = grid.getLayer<float>(LayerId::Elevation);

        GridIndexType center = size / 2;

        for (FlatVectorIndexType i = 0; i < grid.cellCount(); ++i) {
            GridIndexType r = i / size;
            GridIndexType c = i % size;

            // Muros perimetrales
            if (r == 0 || r == size - 1 || c == 0 || c == size - 1) {
                elevationData[i] = 100.0f;
                waterData[i] = 0.0f;
            }
            else {
                elevationData[i] = 0.0f; // Suelo plano

                // Solo ponemos agua en la celda estrictamente central
                if (r == center && c == center) {
                    waterData[i] = 10.0f; // 10 metros de agua de golpe
                }
                else {
                    waterData[i] = 0.0f;
                }
            }
        }
    }

} // namespace danasim
