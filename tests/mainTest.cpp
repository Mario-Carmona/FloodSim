/**
 * @file mainTest.cpp
 * @brief Entry point for the Google Test framework.
 *
 * This file contains the main execution routine for the test suite. It initializes
 * Google Test, sets up the application's custom logger to a restricted level
 * (to avoid noisy test output), runs all registered tests, and then safely shuts
 * down the logging system.
 */

#include <gtest/gtest.h>
#include "logging/Logger.hpp"

 /**
  * @brief Main function to execute all unit tests.
  *
  * Initializes the testing framework and configures the global logger to only
  * report errors (disabling console, file, and debug outputs by default).
  * After test execution, it ensures the logger is properly shut down.
  *
  * @param argc The number of command-line arguments.
  * @param argv The array of command-line arguments.
  * @return int The status code of the test execution (0 if all tests pass, non-zero otherwise).
  */
int main(int argc, char** argv) {
    // Initialize the Google Test framework with command-line arguments
    ::testing::InitGoogleTest(&argc, argv);

    // Initialize the custom logger for the test environment
    // Configured to only show errors, avoiding console or file spam
    floodsim::logging::Logger::Init(
        floodsim::logging::LoggerLevel::kError,
        false,
        false,
        false,
        std::filesystem::path(),
        nullptr
    );

    // Execute all registered Google Tests
    auto result = RUN_ALL_TESTS();

    // Clean up logging resources before exiting
    floodsim::logging::Logger::Shutdown();

    return result;
}
