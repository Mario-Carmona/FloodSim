
#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <filesystem>

#include "app/core/grid/GridMetadata.hpp"

namespace danasim {

    class Writer {
    public:
        Writer() = default;
        virtual ~Writer() = default;

        virtual void save(const std::filesystem::path& /* dataPath */, const std::vector<float>& /* data */, const GridMetadata& /* metadata */) const {
            throw std::runtime_error("Save for float not implemented in this writer");
        }

        virtual void save(const std::filesystem::path& /* dataPath */, const std::vector<int8_t>& /* data */, const GridMetadata& /* metadata */) const {
            throw std::runtime_error("Save for int8_t not implemented in this writer");
        }
    };

} // namespace danasim
