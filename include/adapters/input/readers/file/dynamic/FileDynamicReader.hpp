
#pragma once

#include "adapters/input/readers/DynamicReader.hpp"

#include <string>
#include <filesystem>

namespace danasim {

    class FileDynamicReader : public DynamicReader {
    public:
        enum class Format : uint8_t {
            HIF
        };

        FileDynamicReader(const std::filesystem::path& dataPath, const std::string& dataFilename);
        virtual ~FileDynamicReader() = default;

    protected:
        std::filesystem::path dataPath_;
        std::string dataFilename_;
    };

} // namespace danasim
