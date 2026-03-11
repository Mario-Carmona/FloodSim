
#pragma once

#include <string>
#include <filesystem>
#include <memory>

#include "adapters/input/readers/StaticReader.hpp"

namespace danasim {

    class FileStaticReaderFactory {
    public:
        static std::unique_ptr<StaticReader> create(
            const std::string& format, const std::filesystem::path& dataPath, 
            const std::string& dataFilename
        );
    };

} // namespace danasim
