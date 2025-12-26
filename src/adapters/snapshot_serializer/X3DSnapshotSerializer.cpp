
#include "adapters/snapshot_serializer/X3DSnapshotSerializer.hpp"

#include <fstream>
#include <format>
#include <charconv>
#include <sstream>

namespace danasim {

    X3DSnapshotSerializer::X3DSnapshotSerializer(ColorConfig colorConfig)
        : colorConfig_(colorConfig) {}

    void X3DSnapshotSerializer::serializeToStr(const Snapshot& snapshot, std::string& result) const {
        if (!snapshot.isValid()) {
            throw std::runtime_error("Intento de serializar un Snapshot inválido.");
        }

        // 2. RESERVA Y OPTIMIZACIÓN
        result.reserve(snapshot.grid()->cellCount() * 10 + 2048);

        // 3. FLUJO CENTRALIZADO (C++20 move-to-stream)
        std::ostringstream oss(std::move(result));
        serializeToStream(oss, snapshot);
    }

    void X3DSnapshotSerializer::serializeToFile(const Snapshot& snapshot, const std::filesystem::path& path) const {

    }

    void X3DSnapshotSerializer::serializeToStream(std::ostream& os, const Snapshot& snapshot) const {
        const auto& grid = *snapshot.grid();

        // Boilerplate inicial del HTML/X3D usando std::format
        os << std::format(R"(<!DOCTYPE html>\n<html>\n\t<head>
            \n\t\t<script src="https://www.x3dom.org/download/x3dom.js"></script>
            \n\t\t<link rel="stylesheet" href="https://www.x3dom.org/download/x3dom.css">
            \n\t</head>\n\t<body>\n\t\t<x3d width='100%' height='100%'>\n\t\t\t<scene>)");

        // Capa de Terreno
        os << std::format(R"(\n\t\t\t\t<shape>\n\t\t\t\t\t<appearance>\n\t\t\t\t\t\t<material diffuseColor='{} {} {}'></material>\n\t\t\t\t\t</appearance>
            \n\t\t\t\t\t<elevationGrid xDimension='{}' zDimension='{}' height=')",
            colorConfig_.terrainColor[0], colorConfig_.terrainColor[1], colorConfig_.terrainColor[2],
            grid.width(), grid.height());

        writeHeights(os, grid.layerData(LayerId::Elevation), grid.cellCount());

        os << ">\n\t\t\t\t\t</elevationGrid>\n\t\t\t\t</shape>";

        // Capa de Agua (Simplificado para el ejemplo)
        os << std::format(R"(<transform translation='0 0.01 0'><shape><appearance>
            <material diffuseColor='{} {} {}' transparency='{}'></material></appearance>
            <elevationGrid xDimension='{}' zDimension='{}' height=')",
            colorConfig_.waterColor[0], colorConfig_.waterColor[1], colorConfig_.waterColor[2], colorConfig_.transparency,
            grid.width(), grid.height());

        writeHeights(os, grid.layerData(LayerId::WaterDepth), grid.cellCount());

        os << "></elevationGrid></shape></transform></scene></x3d></body></html>";
    }

    void X3DSnapshotSerializer::writeHeights(std::ostream& os, const float* data, uint32_t count) const {
        // Buffer temporal para un solo número (suficiente para cualquier float)
        char buffer[32];

        for (uint32_t i = 0; i < count; ++i) {
            // OPTIMIZACIÓN 2: std::to_chars
            // Escribe el float en el buffer temporal con formato fixed (f) y 3 decimales
            auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), data[i], std::chars_format::fixed, 3);

            os.write(buffer, ptr - buffer);

            // Separador o salto de línea para legibilidad del XML
            if (i % 20 == 19) {
                os.put('\n');
            }
            else {
                os.put(' ');
            }
        }
    }

} // namespace danasim
