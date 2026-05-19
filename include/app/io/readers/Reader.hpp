
#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <filesystem>

#include "app/core/grid/GridMetadata.hpp"

namespace danasim {

    class Reader {
    public:
        Reader() = default;
        virtual ~Reader() = default;

        virtual GridMetadata readMetadata() const = 0;

        virtual void read(std::vector<float>& /* data */) const {
            throw std::runtime_error("Read for float not implemented in this reader");
        }

        virtual void read(std::vector<int8_t>& /* data */) const {
            throw std::runtime_error("Read for int8_t not implemented in this reader");
        }
    };

} // namespace danasim
