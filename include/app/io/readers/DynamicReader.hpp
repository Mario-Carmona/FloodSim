
#pragma once

#include <memory>
#include <chrono>

#include "app/io/readers/Reader.hpp"

namespace danasim {

    class DynamicReader : public Reader {
    public:
        DynamicReader() = default;
        virtual ~DynamicReader() = default;

        virtual bool update(std::chrono::system_clock::time_point currentTime) = 0;

        virtual unsigned int getDowngradeFactor() const = 0;
    };

} // namespace danasim
