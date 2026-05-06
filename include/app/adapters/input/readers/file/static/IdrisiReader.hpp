
#pragma once

#include "app/adapters/input/readers/file/static/FileStaticReader.hpp"

namespace danasim {

    class IdrisiReader : public FileStaticReader {
    public:
        static bool isStaticLayer(const std::filesystem::path& dataPath, const std::string& dataFilename);

        IdrisiReader(const std::filesystem::path& dataPath, const std::string& dataFilename);
        virtual ~IdrisiReader() = default;

        void read(std::vector<float>& data) const override;
        void read(std::vector<int8_t>& data) const override;

        template <typename T>
        void read(std::vector<T>& data) const;

        GridMetadata readMetadata() const;
    };

} // namespace danasim
