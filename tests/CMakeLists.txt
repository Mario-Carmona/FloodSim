
# Find the Google Test package (required for testing)
find_package(GTest CONFIG REQUIRED)

# Include the GoogleTest CMake module (enables functions like gtest_discover_tests)
include(GoogleTest)

# Compile mainTest.cpp and helpers into a reusable static mini-library
add_library(floodsim_test_main 
    STATIC 
        mainTest.cpp
        helpers/GridTestBuilder.cpp
)

# Link Google Test and project-specific options to our test library
target_link_libraries(floodsim_test_main PUBLIC GTest::gtest floodsim_options)

# Add the tests directory to the include paths
target_include_directories(floodsim_test_main
    PUBLIC
        ${CMAKE_SOURCE_DIR}/tests
)

# Include the adapters subdirectory for further processing
add_subdirectory(adapters)
