
#include "app/io/readers/file/static/IdrisiReader.hpp"

#include <fmt/core.h>
#include <fstream>
#include <cstdlib>
#include <charconv> // Fundamental para std::from_chars
#include <cctype>   // Para std::isspace

#include "logging/Logger.hpp"

namespace danasim {

    bool IdrisiReader::isStaticLayer(const std::filesystem::path& dataPath, const std::string& dataFilename) {
        return std::filesystem::exists(dataPath / (dataFilename + ".img"));
    }

    IdrisiReader::IdrisiReader(const std::filesystem::path& dataPath, const std::string& dataFilename)
        : FileStaticReader(dataPath, dataFilename)
    {

    }

    void IdrisiReader::read(std::vector<float>& data) const {
        read<float>(data);
    }

    void IdrisiReader::read(std::vector<int8_t>& data) const {
        read<int8_t>(data);
    }

    template <typename T>
    void IdrisiReader::read(std::vector<T>& data) const
    {
        std::filesystem::path imgFile = dataPath_ / (dataFilename_ + ".img");

        // Usamos ios::binary y ios::ate para obtener el tamaño exacto sin traducciones de salto de línea de Windows
        std::ifstream file(imgFile, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            throw std::runtime_error(fmt::format("No se pudo abrir el archivo de datos: {}", imgFile.string()));
        }

        // 1. Averiguar el tamaño del archivo y leerlo TODO a la RAM en un solo paso
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::string fileBuffer(size, '\0');
        if (!file.read(fileBuffer.data(), size)) {
            throw std::runtime_error("Error leyendo el archivo a memoria");
        }
        file.close(); // Liberamos el disco de inmediato

        // 2. Parseo de Ultra-Alto Rendimiento desde la RAM
        const char* ptr = fileBuffer.data();
        const char* end = fileBuffer.data() + fileBuffer.size();

        for (size_t i = 0; i < data.size(); ++i) {
            // std::from_chars es tan puro que no ignora los espacios en blanco. Debemos saltarlos manualmente.
            while (ptr < end && std::isspace(static_cast<unsigned char>(*ptr))) {
                ++ptr;
            }

            // Si llegamos al final del buffer antes de llenar el vector, el archivo está incompleto
            if (ptr == end) {
                throw std::runtime_error(fmt::format("Faltan datos en el archivo. Esperados: {}, Encontrados: {}", data.size(), i));
            }

            // Magia de C++17: Convierte el texto al tipo exacto de data[i] sin alojar memoria ni chequear locale
            auto result = std::from_chars(ptr, end, data[i]);

            if (result.ec != std::errc()) {
                throw std::runtime_error(fmt::format("Error de formato leyendo la celda {}", i));
            }

            // Avanzar el puntero justo al final del número que acabamos de leer
            ptr = result.ptr;
        }
	}

    GridMetadata IdrisiReader::readMetadata() const {
        std::filesystem::path docFile = dataPath_ / (dataFilename_ + ".doc");

        std::ifstream file(docFile);
        if (!file.is_open()) {
            throw std::runtime_error(fmt::format("No se pudo abrir el archivo de metadatos: {}", docFile.string()));
        }

        GridMetadata metadata{};
        std::string line;

        // Leemos línea por línea buscando las claves
        while (std::getline(file, line)) {
            // Los archivos .doc de Idrisi tienen el formato: "clave     : valor"
            auto delimiterPos = line.find(':');
            if (delimiterPos == std::string::npos) continue;

            std::string key = line.substr(0, delimiterPos);
            std::string value = line.substr(delimiterPos + 1);

            // Limpiamos espacios en blanco de la clave y el valor (trimming básico)
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));

            // Asignamos según la clave
            if (key == "columns")       metadata.width = std::stoi(value);
            if (key == "rows")          metadata.height = std::stoi(value);
            if (key == "ref. system")   metadata.crs = value;
            if (key == "ref. units")    metadata.units = value;
            if (key == "unit dist.")    metadata.unitDist = std::stof(value);
            if (key == "min. X")        metadata.minX = std::stof(value);
            if (key == "max. X")        metadata.maxX = std::stof(value);
            if (key == "min. Y")        metadata.minY = std::stof(value);
            if (key == "max. Y")        metadata.maxY = std::stof(value);
            if (key == "resolution")    metadata.cellSize = std::stof(value);
        }

        metadata.cellCount = metadata.width * metadata.height;

        return metadata;
    }

} // namespace danasim
