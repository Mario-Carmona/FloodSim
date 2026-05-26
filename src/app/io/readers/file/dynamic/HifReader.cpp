/**
 * @file HifReader.cpp
 * @brief Implementation details for the HifReader class.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/io/readers/file/dynamic/HifReader.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>

#include <fmt/core.h>
#include <nlohmann/json.hpp>

#include "app/io/readers/file/static/IdrisiReader.hpp"
#include "misc/TimeUtils.hpp"

namespace floodsim {

HifReader::HifReader(const std::filesystem::path& data_path,
                     const std::string& data_filename)
    : FileDynamicReader(data_path, data_filename), current_frame_(0) {

    std::filesystem::path json_path = data_path_ / (data_filename + "_metadata.json");
    std::ifstream json_metadata(json_path);

    if (!json_metadata.is_open()) {
        throw std::runtime_error(fmt::format(
            "Failed to open HIF metadata file: {}", json_path.string()));
    }

    nlohmann::json data;
    try {
        data = nlohmann::json::parse(json_metadata);
    }
    catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error(fmt::format(
            "JSON parsing error in {}: {}", json_path.string(), e.what()));
    }

    steps_ = data.value("steps", 0);
    downgrade_factor_ = data.value("downgrade_factor", 1);

    for (const std::string& ts : data["timestamps"]) {
        timestamps_.push_back(ParseTimestampString(ts));
    }

    filenames_ = data["filenames"].get<std::vector<std::string>>();
}

void HifReader::Read(std::vector<float>& data) const {
    ReadData<float>(data);
}

void HifReader::Read(std::vector<int8_t>& data) const {
    ReadData<int8_t>(data);
}

template <typename T>
void HifReader::ReadData(std::vector<T>& data) const {
    if (filenames_.empty()) {
        throw std::runtime_error("No frames available in HIF reader.");
    }

    IdrisiReader reader(data_path_, filenames_[current_frame_]);
    reader.Read(data);
}

GridMetadata HifReader::ReadMetadata() const {
    if (filenames_.empty()) {
        throw std::runtime_error("Cannot read metadata: No frames available in HIF.");
    }

    std::filesystem::path doc_file = data_path_ / (data_filename_ + ".hif");
    std::ifstream file(doc_file);

    if (!file.is_open()) {
        throw std::runtime_error(fmt::format(
            "Failed to open metadata file: {}", doc_file.string()));
    }

    GridMetadata metadata{};
    std::string line;

    while (std::getline(file, line)) {
        auto delimiter_pos = line.find(':');
        if (delimiter_pos == std::string::npos) continue;

        std::string key = line.substr(0, delimiter_pos);
        std::string value = line.substr(delimiter_pos + 1);

        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));

        if (key == "columns") metadata.width = std::stoi(value);
        if (key == "rows") metadata.height = std::stoi(value);
        if (key == "min. X") metadata.min_x = std::stof(value);
        if (key == "max. X") metadata.max_x = std::stof(value);
        if (key == "min. Y") metadata.min_y = std::stof(value);
        if (key == "max. Y") metadata.max_y = std::stof(value);
        if (key == "resolution") metadata.cell_size = std::stof(value);
        if (key == "ref. system") metadata.crs = value;
    }

    metadata.cell_count = metadata.width * metadata.height;
    return metadata;
}

bool HifReader::Update(std::chrono::system_clock::time_point current_time) {
    bool updated = false;
        
    while (current_time >= timestamps_[current_frame_ + 1] && current_frame_ < filenames_.size()) {
        current_frame_++;
        updated = true;
    }

    return updated;
}

unsigned int HifReader::GetDowngradeFactor() const {
    return downgrade_factor_;
}

} // namespace floodsim
