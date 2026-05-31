
# Ejecutable del test
add_executable(danasim_state_updater_adapters_test
    StateUpdaterTest.cpp
)

# Enlazamos la librería principal, GTest y GMock
target_link_libraries(danasim_state_updater_adapters_test 
    PRIVATE 
        danasim_state_updater_adapters
        danasim_test_main 
        GTest::gmock
)

# Descubrimiento automático para CTest
gtest_discover_tests(danasim_state_updater_adapters_test DISCOVERY_MODE PRE_TEST)
