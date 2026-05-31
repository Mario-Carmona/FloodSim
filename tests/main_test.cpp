
#include <gtest/gtest.h>

#include "logging/Logger.hpp"

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);

    danasim::Logger::init(
        "error",
        false,
        false,
        false,
        ""
    );

    auto result = RUN_ALL_TESTS();

    danasim::Logger::shutdown();

    return result;
}
