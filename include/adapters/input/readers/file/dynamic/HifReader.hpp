
#pragma once

#include "adapters/input/readers/file/dynamic/FileDynamicReader.hpp"

#include <vector>
#include <iostream>
#include <chrono>
#include <sstream>
#include <string>

namespace danasim {

    class HifReader : public FileDynamicReader {
    public:
        HifReader(const std::filesystem::path& dataPath, const std::string& dataFilename);
        virtual ~HifReader() = default;

        void read(std::vector<float>& data) const override;
        void read(std::vector<int8_t>& data) const override;

        GridMetadata readMetadata() const override;

        bool update(std::chrono::system_clock::time_point currentTime) override;

        unsigned int getDowngradeFactor() const override;

    protected:
        unsigned int steps_;
        unsigned int downgradeFactor_;
        std::vector<std::chrono::system_clock::time_point> timestamps_;
        std::vector<std::string> filenames_;

        unsigned int currentFrame_;

        template <typename T>
        void read(std::vector<T>& data) const;
    };

} // namespace danasim
