
#pragma once

#include "app/adapters/input/readers/StaticReader.hpp"

#include <string>
#include <filesystem>

#include "app/adapters/input/readers/file/static/FileStaticFormat.hpp"

namespace danasim {

    class FileStaticReader : public StaticReader {
    public:
        static bool isStaticLayer(const std::filesystem::path& dataPath, const std::string& dataFilename, const FileStaticFormat& format);


        FileStaticReader(const std::filesystem::path& dataPath, const std::string& dataFilename);
        virtual ~FileStaticReader() = default;

    protected:
        std::filesystem::path dataPath_;
        std::string dataFilename_;
    };

} // namespace danasim
