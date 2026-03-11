
#include "adapters/input/readers/file/dynamic/HifReader.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <algorithm>
#include <fmt/core.h>

#include "misc/Time.hpp"
#include "adapters/input/readers/file/static/IdrisiReader.hpp"

namespace danasim {

    HifReader::HifReader(const std::filesystem::path& dataPath, const std::string& dataFilename)
        : FileDynamicReader(dataPath, dataFilename)
    {
        std::ifstream attrs(dataPath_ / "attrs.json");
        nlohmann::json data = nlohmann::json::parse(attrs);

        steps_ = data["steps"];
        downgradeFactor_ = data["downgrade_factor"];

        for (const std::string& ts : data["timestamps"]) {
            timestamps_.push_back(parseTimestampString(ts));
        }

        filenames_ = data["filenames"].get<std::vector<std::string>>();

        currentFrame_ = 0;
    }

    void HifReader::read(std::vector<float>& data) const {
        read<float>(data);
    }

    void HifReader::read(std::vector<int8_t>& data) const {
        read<int8_t>(data);
    }

    template <typename T>
    void HifReader::read(std::vector<T>& data) const
    {
        if(currentFrame_ == filenames_.size()) {
            std::fill(data.begin(), data.end(), T{});
        }
        else {
            IdrisiReader frameReader(dataPath_, filenames_[currentFrame_]);

            frameReader.read(data);
        }
	}

    GridMetadata HifReader::readMetadata() const {
        std::filesystem::path hifFile = dataPath_ / (dataFilename_ + ".hif");

        std::ifstream file(hifFile);
        if (!file.is_open()) {
            throw std::runtime_error(fmt::format("No se pudo abrir el archivo de metadatos: {}", hifFile.string()));
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
            if (key == "columns")    metadata.width = std::stoi(value);
            if (key == "rows")       metadata.height = std::stoi(value);
            if (key == "min. X")     metadata.minX = std::stof(value);
            if (key == "max. X")     metadata.maxX = std::stof(value);
            if (key == "min. Y")     metadata.minY = std::stof(value);
            if (key == "max. Y")     metadata.maxY = std::stof(value);
            if (key == "resolution") metadata.cellSize = std::stof(value);
            if (key == "ref. system") metadata.crs = value;
        }

        metadata.cellCount = metadata.width * metadata.height;

        return metadata;
    }

    bool HifReader::update(std::chrono::system_clock::time_point currentTime) {
        bool updated = false;
        
        while (currentTime >= timestamps_[currentFrame_ + 1] && currentFrame_ < filenames_.size()) {
            currentFrame_++;
            updated = true;
        }

        return updated;
    }

    unsigned int HifReader::getDowngradeFactor() const {
        return downgradeFactor_;
    }

} // namespace danasim
