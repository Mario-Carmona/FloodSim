
#pragma once

#include <string>
#include <filesystem>
#include <memory>

#include "app/io/formats/file/FileStaticFormat.hpp"
#include "app/io/writers/StaticWriter.hpp"

namespace danasim {

    class FileStaticWriterFactory {
    public:
        static std::unique_ptr<StaticWriter> create(const FileStaticFormat& format, const std::string& dataFilename);
    };

} // namespace danasim
