/**
 * @file IdrisiWriter.cpp
 * @brief Implementation details for the IdrisiWriter class.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/io/writers/file/static/IdrisiWriter.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>

#include <fmt/core.h>

#include "logging/Logger.hpp"

namespace floodsim {

IdrisiWriter::IdrisiWriter(const std::string& data_filename)
    : FileStaticWriter(data_filename) {}

void IdrisiWriter::Save(const std::filesystem::path& data_path,
                        const std::vector<float>& data,
                        const GridMetadata& metadata) const {
    SaveData<float>(data_path, data, metadata);
}

void IdrisiWriter::Save(const std::filesystem::path& data_path,
                        const std::vector<int8_t>& data,
                        const GridMetadata& metadata) const {
    SaveData<int8_t>(data_path, data, metadata);
}

template <typename T>
void IdrisiWriter::SaveData(const std::filesystem::path& data_path,
                            const std::vector<T>& data,
                            const GridMetadata& metadata) const {
    std::filesystem::path img_filepath = data_path / (data_filename_ + ".img");
    std::filesystem::path doc_filepath = data_path / (data_filename_ + ".doc");

    // 1. Write the data file (.img)
    // We use `ios::binary` to prevent the OS from performing slow conversions of line breaks (from `\n` to `\r\n`)
    std::ofstream img_file(img_filepath, std::ios::binary);
    if (!img_file.is_open()) {
        throw std::runtime_error(fmt::format(
            "Failed to open IDRISI image file for writing: {}", img_filepath.string()));
    }

    // High-performance write buffer using std::to_chars
    std::array<char, 64> buffer;
    for (size_t i = 0; i < data.size(); ++i) {
        auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), data[i]);
        if (ec == std::errc()) {
            img_file.write(buffer.data(), ptr - buffer.data());
            img_file.put((i % metadata.width == metadata.width - 1) ? '\n' : ' ');
        }
        else {
            throw std::runtime_error(fmt::format("Format error writing cell {}", i));
        }
    }
    img_file.close();

    // 2. Create the documentation file (.doc)
    std::ofstream doc_file(doc_filepath);
    if (!doc_file.is_open()) {
        throw std::runtime_error(fmt::format(
            "Failed to open IDRISI doc file for writing: {}", doc_filepath.string()));
    }

    T min_value = *std::min_element(data.begin(), data.end());
    T max_value = *std::max_element(data.begin(), data.end());

    // Key format: value required by IDRISI
    doc_file << "file format : IDRISI Raster A.1\n";
    doc_file << "file title  : \n";
    doc_file << "data type   : real\n";
    doc_file << "file type   : ascii\n";
    doc_file << "columns     : " << metadata.width << "\n";
    doc_file << "rows        : " << metadata.height << "\n";
    doc_file << "ref. system : " << metadata.crs << "\n";
    doc_file << "ref. units  : " << metadata.units << "\n";
    doc_file << "unit dist.  : " << metadata.unit_dist << "\n";
    doc_file << "min. X      : " << metadata.min_x << "\n";
    doc_file << "max. X      : " << metadata.max_x << "\n";
    doc_file << "min. Y      : " << metadata.min_y << "\n";
    doc_file << "max. Y      : " << metadata.max_y << "\n";
    doc_file << "pos'n error : unknown\n";
    doc_file << "resolution  : " << metadata.cell_size << "\n";
    doc_file << "min. value  : " << min_value << "\n";
    doc_file << "max. value  : " << max_value << "\n";
    doc_file << "display min : " << min_value << "\n";
    doc_file << "display max : " << max_value << "\n";
    doc_file << "value units : " << "unspecified" << "\n";
    doc_file << "nodata value: " << -9999.0 << "\n";

    doc_file.close();
}

} // namespace floodsim
