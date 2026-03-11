
#pragma once

#include <string>
#include <vector>
#include <stdexcept>

namespace danasim {

    struct GridMetadata {
        int width;
        int height;
        int cellCount;

        float cellSize;

        double minX;
        double maxX;
        double minY;
        double maxY;

        std::string crs;
    };

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
