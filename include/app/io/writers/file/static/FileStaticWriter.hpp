
#pragma once

#include "app/io/writers/StaticWriter.hpp"

#include <string>
#include <filesystem>

namespace danasim {

    class FileStaticWriter : public StaticWriter {
    public:
        FileStaticWriter(const std::string& dataFilename);
        virtual ~FileStaticWriter() = default;

    protected:
        std::string dataFilename_;
    };

} // namespace danasim
