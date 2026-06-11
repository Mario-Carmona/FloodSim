
# Define the test executable and its source files
add_executable(floodsim_state_updater_adapters_test
    StateUpdaterTest.cpp
)

# Link the test executable with the main adapter library, the test setup library, and GMock
target_link_libraries(floodsim_state_updater_adapters_test 
    PRIVATE 
        floodsim_state_updater_adapters
        floodsim_test_main 
        GTest::gmock
)

# Enable automatic test discovery for CTest
gtest_discover_tests(floodsim_state_updater_adapters_test DISCOVERY_MODE PRE_TEST)
