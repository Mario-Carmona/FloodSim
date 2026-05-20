
#pragma once

#include <vector>

#include "app/io/writers/Writer.hpp"

namespace danasim {

    class StaticWriter : public Writer {
    public:
        StaticWriter() = default;
        virtual ~StaticWriter() = default;
    };

} // namespace danasim
