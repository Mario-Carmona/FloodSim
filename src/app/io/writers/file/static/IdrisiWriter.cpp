
#include "app/io/writers/file/static/IdrisiWriter.hpp"

#include <fmt/core.h>
#include <fstream>
#include <cstdlib>
#include <charconv> // Fundamental para std::from_chars
#include <cctype>   // Para std::isspace
#include <algorithm>

#include "logging/Logger.hpp"

namespace danasim {

    IdrisiWriter::IdrisiWriter(const std::string& dataFilename)
        : FileStaticWriter(dataFilename)
    {

    }

    void IdrisiWriter::save(const std::filesystem::path& dataPath, const std::vector<float>& data, const GridMetadata& metadata) const {
        save<float>(dataPath, data, metadata);
    }

    void IdrisiWriter::save(const std::filesystem::path& dataPath, const std::vector<int8_t>& data, const GridMetadata& metadata) const {
        save<int8_t>(dataPath, data, metadata);
    }

    template <typename T>
    void IdrisiWriter::save(const std::filesystem::path& dataPath, const std::vector<T>& data, const GridMetadata& metadata) const
    {
        std::filesystem::path imgFilepath = dataPath / (dataFilename_ + ".img");

        // Usamos ios::binary para evitar que el SO haga traducciones lentas de saltos de línea (\n a \r\n)
        std::ofstream imgFile(imgFilepath, std::ios::binary);
        if (!imgFile.is_open()) {
            throw std::runtime_error(fmt::format("No se pudo crear el archivo de datos: {}", imgFilepath.string()));
        }

        // 1. Escritura de Ultra-Alto Rendimiento en memoria
        // Reservamos espacio para evitar realojos de memoria constantes.
        // Un float convertido a texto ocupa como máximo ~15 caracteres, un int8_t ~4-5 caracteres.
        size_t estimatedCharsPerCell = std::is_floating_point_v<T> ? 15 : 5;

        std::string writeBuffer;
        writeBuffer.reserve(data.size() * estimatedCharsPerCell);

        // Buffer temporal estático para std::to_chars
        std::array<char, 64> tempBuffer;

        for (size_t i = 0; i < data.size(); ++i) {
            // Magia de C++17: Conversión rápida de número a texto
            auto result = std::to_chars(tempBuffer.data(), tempBuffer.data() + tempBuffer.size(), data[i]);

            if (result.ec != std::errc()) {
                throw std::runtime_error(fmt::format("Error de formato procesando la celda para escritura {}", i));
            }

            // Añadir el número convertido al buffer principal
            writeBuffer.append(tempBuffer.data(), result.ptr);

            // Añadir un espacio en blanco como separador (compatible con std::isspace en la lectura)
            writeBuffer.push_back(' ');
        }

        // 2. Volcado a disco en un solo paso
        if (!imgFile.write(writeBuffer.data(), writeBuffer.size())) {
            throw std::runtime_error("Error volcando el buffer de datos al disco");
        }

        // Liberamos el archivo explícitamente
        imgFile.close();


        std::filesystem::path docFilepath = dataPath / (dataFilename_ + ".doc");

        // Aquí no hace falta ios::binary porque es un archivo pequeño y puramente textual
        std::ofstream docFile(docFilepath);
        if (!docFile.is_open()) {
            throw std::runtime_error(fmt::format("No se pudo crear el archivo de metadatos: {}", docFilepath.string()));
        }

        // Respetamos el formato de espaciado que leímos: "clave     : valor"
        docFile << "file format : " << "IDRISI Raster A.1" << "\n";
        docFile << "file title  : " << "" << "\n";
        docFile << "data type   : " << "real" << "\n";
        docFile << "file type   : " << "ascii" << "\n";
        docFile << "columns     : " << metadata.width << "\n";
        docFile << "rows        : " << metadata.height << "\n";
        docFile << "ref. system : " << metadata.crs << "\n";
        docFile << "ref. units  : " << metadata.units << "\n";
        docFile << "unit dist.  : " << metadata.unitDist << "\n";
        docFile << "min. X      : " << metadata.minX << "\n";
        docFile << "max. X      : " << metadata.maxX << "\n";
        docFile << "min. Y      : " << metadata.minY << "\n";
        docFile << "max. Y      : " << metadata.maxY << "\n";
        docFile << "pos'n error : " << "unknown" << "\n";
        docFile << "resolution  : " << metadata.cellSize << "\n";

        T minValue = *std::min_element(data.begin(), data.end());
        T maxValue = *std::max_element(data.begin(), data.end());

        docFile << "min. value  : " << minValue << "\n";
        docFile << "max. value  : " << maxValue << "\n";
        docFile << "display min : " << minValue << "\n";
        docFile << "display max : " << maxValue << "\n";

        docFile << "value units : " << "unspecified" << "\n";
        docFile << "nodata value: " << -9999.0 << "\n";

        docFile.close();
    }

} // namespace danasim
