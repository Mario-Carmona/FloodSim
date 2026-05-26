/**
 * @file IdrisiReader.cpp
 * @brief Implementation details for the IdrisiReader class.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/io/readers/file/static/IdrisiReader.hpp"

#include <cctype>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>

#include <fmt/core.h>

#include "logging/Logger.hpp"

namespace floodsim::app::io::readers::file {

bool IdrisiReader::IsStaticLayer(const std::filesystem::path& data_path,
                                 const std::string& data_filename) {
    return std::filesystem::exists(data_path / (data_filename + ".img"));
}

IdrisiReader::IdrisiReader(const std::filesystem::path& data_path,
                           const std::string& data_filename)
    : FileStaticReader(data_path, data_filename) {}

void IdrisiReader::Read(std::vector<float>& data) const {
    ReadData<float>(data);
}

void IdrisiReader::Read(std::vector<int8_t>& data) const {
    ReadData<int8_t>(data);
}

template <typename T>
void IdrisiReader::ReadData(std::vector<T>& data) const {
    std::filesystem::path img_file = data_path_ / (data_filename_ + ".img");

    // We use `ios::binary` and `ios::ate` to get the exact size without line break translations
    std::ifstream file(img_file, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error(fmt::format(
            "Failed to open data file: {}", img_file.string()));
    }

    // 1. Determine the file size and load the ENTIRE file into RAM in a single step
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::string file_buffer(size, '\0');
    if (!file.read(file_buffer.data(), size)) {
        throw std::runtime_error("Error reading file into memory buffer.");
    }
    file.close();  // We'll release the album right away

    // 2. Ultra-High-Performance Parsing from RAM
    const char* ptr = file_buffer.data();
    const char* end = file_buffer.data() + file_buffer.size();

    for (size_t i = 0; i < data.size(); ++i) {
        // std::from_chars does not ignore whitespace. We must skip it manually.
        while (ptr < end && std::isspace(static_cast<unsigned char>(*ptr))) {
            ++ptr;
        }

        // If we reach the end of the buffer before filling the vector, the file is incomplete
        if (ptr == end) {
            throw std::runtime_error(fmt::format(
                "Missing data in file. Expected: {}, Found: {}", data.size(), i));
        }

        // Convert the text to the exact data type[i] without allocating memory or checking the locale
        auto result = std::from_chars(ptr, end, data[i]);

        if (result.ec != std::errc()) {
            throw std::runtime_error(fmt::format("Format error reading cell {}", i));
        }

        // Move the cursor right to the end of the number we just read
        ptr = result.ptr;
    }
}

core::grid::GridMetadata IdrisiReader::ReadMetadata() const {
    std::filesystem::path doc_file = data_path_ / (data_filename_ + ".doc");

    std::ifstream file(doc_file);
    if (!file.is_open()) {
        throw std::runtime_error(fmt::format(
            "Failed to open metadata file: {}", doc_file.string()));
    }

    core::grid::GridMetadata metadata{};
    std::string line;

    // We read line by line, looking for clues
    while (std::getline(file, line)) {
        // Idrisi .doc files use the following format: “key     : value”
        auto delimiter_pos = line.find(':');
        if (delimiter_pos == std::string::npos) continue;

        std::string key = line.substr(0, delimiter_pos);
        std::string value = line.substr(delimiter_pos + 1);

        // We remove leading and trailing spaces from the key and value
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));

        // We assign based on the key
        if (key == "columns") metadata.width = std::stoi(value);
        if (key == "rows") metadata.height = std::stoi(value);
        if (key == "ref. system") metadata.crs = value;
        if (key == "ref. units") metadata.units = value;
        if (key == "unit dist.") metadata.unit_dist = std::stof(value);
        if (key == "min. X") metadata.min_x = std::stof(value);
        if (key == "max. X") metadata.max_x = std::stof(value);
        if (key == "min. Y") metadata.min_y = std::stof(value);
        if (key == "max. Y") metadata.max_y = std::stof(value);
        if (key == "resolution") metadata.cell_size = std::stof(value);
    }

    metadata.cell_count = metadata.width * metadata.height;

    return metadata;
}

} // namespace floodsim::app::io::readers::file
