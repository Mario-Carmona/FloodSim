
# Ejecutable del test
add_executable(danasim_core_test
    SimulationCoreTest.cpp
)

# Enlazamos la librería principal, GTest y GMock
target_link_libraries(danasim_core_test 
    PRIVATE 
        danasim_core
        danasim_test_main 
        GTest::gmock
)

# Descubrimiento automático para CTest
gtest_discover_tests(danasim_core_test DISCOVERY_MODE PRE_TEST)
