
#pragma once

#include <vector>

#include "app/adapters/input/readers/Reader.hpp"

namespace danasim {

    class StaticReader : public Reader {
    public:
        StaticReader() = default;
        virtual ~StaticReader() = default;
    };

} // namespace danasim
