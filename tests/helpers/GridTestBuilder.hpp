
#pragma once

#include "core/grid/MapGrid.hpp"

namespace danasim {

    class GridTestBuilder {
    public:
        // 1. Escenario: Lago en reposo (Todo plano, agua uniforme)
        static void createLakeAtRest(MapGrid& grid, GridIndexType rows, GridIndexType cols, float waterDepth);

        // 2. Escenario: Lago en reposo con fondo irregular (Cota de superficie constante)
        static void createIrregularLakeAtRest(MapGrid& grid, GridIndexType rows, GridIndexType cols, float surfaceElevation);

        // 3. Escenario: Rotura de presa en piscina (Agua concentrada en el centro, bordes amurallados)
        static void createDamBreakInPool(MapGrid& grid, GridIndexType rows, GridIndexType cols);

        // 4. Escenario: Gota en el centro para Test de Simetría (Terreno plano, bordes amurallados)
        static void createDropInCenter(MapGrid& grid, GridIndexType size);
    };

} // namespace danasim
