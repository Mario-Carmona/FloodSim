
find_package(GTest CONFIG REQUIRED)

include(GoogleTest)

# 1. Compilamos el main.cpp en una mini-librería reutilizable
add_library(danasim_test_main 
    STATIC 
        main_test.cpp
        helpers/GridTestBuilder.cpp
)


target_link_libraries(danasim_test_main PUBLIC GTest::gtest danasim_options)

target_include_directories(danasim_test_main
    PUBLIC
        ${CMAKE_SOURCE_DIR}/tests
)

if(ENABLE_CORE)
    add_subdirectory(core)
endif()

add_subdirectory(adapters)
