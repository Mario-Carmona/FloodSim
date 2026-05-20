
#pragma once

#include "app/io/writers/file/static/FileStaticWriter.hpp"

namespace danasim {

    class IdrisiWriter : public FileStaticWriter {
    public:
        IdrisiWriter(const std::string& dataFilename);
        virtual ~IdrisiWriter() = default;

        void save(const std::filesystem::path& dataPath, const std::vector<float>& data, const GridMetadata& metadata) const override;
        void save(const std::filesystem::path& dataPath, const std::vector<int8_t>& data, const GridMetadata& metadata) const override;

        template <typename T>
        void save(const std::filesystem::path& dataPath, const std::vector<T>& data, const GridMetadata& metadata) const;
    };

} // namespace danasim
