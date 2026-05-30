/**
 * @file FileStaticReader.hpp
 * @brief Interface for file-based static layer readers.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <filesystem>
#include <string>

#include "app/io/formats/file/FileStaticFormat.hpp"
#include "app/io/readers/StaticReader.hpp"

namespace floodsim::app::io::readers::file {

/**
 * @brief Reader implementation for processing static geographical data files.
 */
class FileStaticReader : public StaticReader {
public:
    /**
     * @brief Validates if the file at the given path matches the specified static format.
     * @param data_path Target directory containing the dataset.
     * @param data_filename File name of the dataset.
     * @param format The format descriptor to check against.
     * @return True if the file matches the layout requirements of the format, false otherwise.
     * @throws std::runtime_error If an unsupported file format is supplied.
     */
    static bool IsStaticLayer(const std::filesystem::path& data_path,
                              const std::string& data_filename,
                              const formats::file::FileStaticFormat& format);

    /**
     * @brief Constructs a new FileStaticReader.
     * @param data_path Target directory containing the data.
     * @param data_filename File name of the target resource.
     */
    FileStaticReader(const std::filesystem::path& data_path,
                     const std::string& data_filename);
    virtual ~FileStaticReader() override = default;

protected:
    /** @brief The path to the directory containing the static files. */
    std::filesystem::path data_path_;
    /** @brief The filename of the static resource. */
    std::string data_filename_;
};

} // namespace floodsim::app::io::readers::file
