
#pragma once

#include <string>
#include <filesystem>
#include <memory>

#include "app/io/formats/file/FileDynamicFormat.hpp"
#include "app/io/readers/DynamicReader.hpp"

namespace danasim {

    class FileDynamicReaderFactory {
    public:
        static std::unique_ptr<DynamicReader> create(
            const FileDynamicFormat& format, const std::filesystem::path& dataPath,
            const std::string& dataFilename
        );
    };

} // namespace danasim
