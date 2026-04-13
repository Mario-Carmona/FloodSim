
#pragma once

#include "adapters/input/readers/StaticReader.hpp"

#include <string>
#include <filesystem>

namespace danasim {

    class FileStaticReader : public StaticReader {
    public:
        enum class Format : uint8_t {
            IDRISI
        };


        static bool isStaticLayer(const std::filesystem::path& dataPath, const std::string& dataFilename, const std::string& formatStr);


        FileStaticReader(const std::filesystem::path& dataPath, const std::string& dataFilename);
        virtual ~FileStaticReader() = default;

    protected:
        std::filesystem::path dataPath_;
        std::string dataFilename_;
    };

} // namespace danasim
