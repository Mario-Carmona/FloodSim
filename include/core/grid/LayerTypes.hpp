
#pragma once
#include <cstdint>

namespace danasim {

    enum class LayerId : uint16_t {
        Elevation,
        Roughness,
        WaterDepth,
        CellState
    };

    enum class LayerCategory : uint8_t {
        Static,
        Dynamic
    };

    enum class LayerRole : uint8_t {
        Principal,
        Secondary 
    };

    namespace CellStateValues {
        static constexpr float Empty = 0.0f;
        static constexpr float Static = 1.0f;
        static constexpr float Dynamic = 2.0f;

        // Valor de tolerancia para comparaciones
        static constexpr float WaterThreshold = 1e-4f;
    }

} // namespace danasim
