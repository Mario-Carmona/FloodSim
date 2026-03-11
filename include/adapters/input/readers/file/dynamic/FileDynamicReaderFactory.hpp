
#pragma once

#include <string>
#include <filesystem>
#include <memory>

#include "adapters/input/readers/DynamicReader.hpp"

namespace danasim {

    class FileDynamicReaderFactory {
    public:
        static std::unique_ptr<DynamicReader> create(
            const std::string& format, const std::filesystem::path& dataPath, 
            const std::string& dataFilename
        );
    };

} // namespace danasim
