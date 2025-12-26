
#include "adapters/state_updater/ExperimentalStateUpdater.hpp"

#include <random>
#include <cstdint>

namespace danasim {

    ExperimentalStateUpdater::ExperimentalStateUpdater() {
        
    }

    ExperimentalStateUpdater::~ExperimentalStateUpdater() {
        
    }

    void ExperimentalStateUpdater::update(MapGrid& grid, float dt) {
        auto& vec_x = grid.changedX();
        auto& vec_y = grid.changedY();


        // 1. Configurar el generador (Solo se hace UNA vez)
        std::random_device rd;  // Obtiene una semilla aleatoria del hardware
        std::mt19937 gen(rd()); // El motor 'Mersenne Twister' (estándar y rápido)
        std::uniform_int_distribution<int32_t> dis(0, 19); // Define el rango [0, 19]

        // 2. Vector para guardar los datos
        std::vector<int32_t> posiciones;

        vec_x.clear();
        vec_y.clear();

        // 3. Generar 5 números
        for (int i = 0; i < 5; ++i) {
            vec_x.push_back(dis(gen));
            vec_y.push_back(dis(gen));
        }
    }

} // namespace danasim
