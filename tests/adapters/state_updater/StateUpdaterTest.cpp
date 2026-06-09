/**
 * @file StateUpdaterTest.cpp
 * @brief Parameterized unit tests to validate the physical consistency of state updaters.
 *
 * This file implements a test suite that dynamically discovers exported machine learning
 * models (e.g., ONNX models) and runs them against standard hydrostatic and physical
 * stability scenarios to ensure they conserve mass and do not produce physical hallucinations.
 */

#include <gtest/gtest.h>
#include <filesystem>
#include <numeric>
#include <stdexcept>

 // Platform-specific includes for resolving the executable path
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#include <limits.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <climits>
#endif

#include "app/adapters/state_updater/StateUpdaterFactory.hpp"
#include "helpers/GridTestBuilder.hpp"
#include "app/core/ports/StateUpdaterPort.hpp"

namespace floodsim::tests {

    namespace {

        /**
         * @brief Retrieves the absolute path to the directory containing the current executable.
         *
         * Uses platform-specific APIs (Windows, Linux, macOS) to resolve the executable's path.
         * This is necessary to locate the external models directory reliably, regardless of
         * the working directory from which the tests are launched.
         *
         * @return std::filesystem::path The directory containing the executable.
         * @throws std::runtime_error If the path cannot be resolved on the current OS.
         */
        std::filesystem::path GetExecutableFolder() {
#if defined(_WIN32)
            wchar_t buffer[MAX_PATH];
            DWORD size = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
            if (size == 0 || size == MAX_PATH) {
                throw std::runtime_error("GetExecutableFolder: Error getting path on Windows.");
            }
            return std::filesystem::path(buffer).parent_path();
#elif defined(__linux__)
            char buffer[PATH_MAX];
            ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
            if (len == -1) {
                throw std::runtime_error("GetExecutableFolder: Error reading /proc/self/exe on Linux.");
            }
            buffer[len] = '\0';
            return std::filesystem::path(buffer).parent_path();
#elif defined(__APPLE__)
            char buffer[PATH_MAX];
            uint32_t size = sizeof(buffer);
            if (_NSGetExecutablePath(buffer, &size) != 0) {
                // If the initial buffer was too small, 'size' now contains the required size
                std::vector<char> dynamic_buffer(size);
                if (_NSGetExecutablePath(dynamic_buffer.data(), &size) != 0) {
                    throw std::runtime_error("GetExecutableFolder: Error getting path on macOS.");
                }
                return std::filesystem::canonical(std::filesystem::path(dynamic_buffer.data())).parent_path();
            }
            return std::filesystem::canonical(std::filesystem::path(buffer)).parent_path();
#else
#error "Platform not supported by GetExecutableFolder"
#endif
        }

    } // anonymous namespace

    /**
     * @struct ModelTestParam
     * @brief Holds configuration parameters for a single parameterized test execution.
     */
    struct ModelTestParam {
        std::string testName;                  ///< The sanitized name of the test (e.g., "ONNX_Model_V1")
        app::config::StateUpdaterConfig config; ///< Configuration object for the state updater factory
    };

    /**
     * @brief Overloads the stream insertion operator for ModelTestParam.
     * * Instructs Google Test (and standard streams) on how to print this structure
     * to the console, ensuring test reports display clear model names.
     *
     * @param os Output stream.
     * @param param The test parameter instance.
     * @return std::ostream& Reference to the modified output stream.
     */
    inline std::ostream& operator<<(std::ostream& os, const ModelTestParam& param) {
        os << param.testName;
        return os;
    }

    /**
     * @brief Dynamically discovers exported machine learning models in the directory tree.
     *
     * Scans the "exported_models" directory relative to the executable path and
     * generates a vector of test parameters for each valid model directory found.
     *
     * @return std::vector<ModelTestParam> A list of parameters representing the discovered models.
     */
    std::vector<ModelTestParam> DiscoverModels() {
        std::vector<ModelTestParam> models;

        // Resolve the absolute path to the exported_models folder
        std::filesystem::path modelsDir = GetExecutableFolder() / "../../../exported_models";

        // Verify that the directory exists and is valid
        if (!std::filesystem::exists(modelsDir) || !std::filesystem::is_directory(modelsDir)) {
            std::cerr << "[WARNING] Model directory not found at: "
                << std::filesystem::absolute(modelsDir) << "\n";
            return models;
        }

        // Iterate ONLY over the direct elements of the base directory
        for (const auto& entry : std::filesystem::directory_iterator(modelsDir)) {

            // If the element is a directory, create a TestParam for it
            if (entry.is_directory()) {

                // Get the folder name (e.g., "first_model_cpu")
                std::string folderName = entry.path().filename().string();

                // Sanitize the name to make it a valid GTest identifier (no spaces or dashes)
                std::replace_if(folderName.begin(), folderName.end(),
                    [](char c) { return !std::isalnum(c); }, '_');

                // Set up the path towards the folder so OnnxStateUpdater can process it
                app::config::OnnxUpdaterConfig onnxConfig;
                onnxConfig.model_path = entry.path(); // Pass the complete folder path
                onnxConfig.tensor_dim = 4;

                models.push_back({
                    "ONNX_" + folderName,
                    onnxConfig
                    });
            }
        }

        return models;
    }

    /**
     * @class StateUpdaterTest
     * @brief Parameterized test fixture for evaluating State Updater implementations.
     *
     * Inherits from ::testing::TestWithParam to allow running the same suite of
     * physical tests across multiple exported models.
     */
    class StateUpdaterTest : public ::testing::TestWithParam<ModelTestParam> {
    protected:
        std::unique_ptr<app::core::ports::StateUpdaterPort> updater; ///< Pointer to the loaded updater interface

        /**
         * @brief Sets up the test environment before each parameterized test execution.
         * * Instantiates the correct updater based on the variant of the provided parameter
         * using the StateUpdaterFactory.
         */
        void SetUp() override {
            updater = app::adapters::state_updater::StateUpdaterFactory::Create(GetParam().config);
        }
    };

    // =========================================================================
    // TEST DEFINITIONS (Executed once for every discovered model)
    // =========================================================================

    /**
     * @brief Verifies that the model initializes correctly without crashing.
     */
    TEST_P(StateUpdaterTest, InitializationDoesNotCrash) {
        app::core::grid::MapGrid grid;

        GridTestBuilder::InitializeGridBase(grid, updater.get(), 10, 10);

        EXPECT_NO_THROW({
            this->updater->Initialize(grid);
            });
    }

    // =========================================================================
    // PHYSICAL TESTS: HYDROSTATIC STABILITY
    // =========================================================================

    /**
     * @brief Verifies hydrostatic stability on a perfectly flat basin.
     * * Ensures that a flat lake at rest remains perfectly still over multiple
     * simulation steps, without generating phantom waves or losing mass.
     */
    TEST_P(StateUpdaterTest, FlatLakeRemainsAtRest) {
        app::core::grid::MapGrid grid;
        // Create a 20x20 lake with 2 meters of depth
        GridTestBuilder::CreateLakeAtRest(grid, 20, 20, 2.0f, updater.get());

        // Store the initial state of the depths to compare later
        std::vector<float> initialWater = grid.GetLayer<float>("water_depth")->GetData();

        // Initialize the model
        this->updater->Initialize(grid);

        // Execute 100 simulation iterations
        for (int step = 0; step < 100; ++step) {
            this->updater->Step(grid);
        }

        const std::vector<float>& finalWater = grid.GetLayer<float>("water_depth")->GetData();

        // Assertion: Depth must not have changed. 
        // We use a 1e-5 tolerance to dismiss float32 precision noise from the neural/physics network.
        for (size_t i = 0; i < initialWater.size(); ++i) {
            EXPECT_NEAR(initialWater[i], finalWater[i], 1e-5f)
                << "Instability detected on flat terrain at cell " << i;
        }
    }

    /**
     * @brief Verifies hydrostatic stability over an irregular bed topology.
     * * Ensures that if the water surface is uniformly flat, variations in
     * the underlying terrain (bathymetry) do not induce false momentum.
     */
    TEST_P(StateUpdaterTest, IrregularLakeRemainsAtRest) {
        app::core::grid::MapGrid grid;
        // Create a 20x20 lake where the water surface is always at Z=5.0m elevation
        GridTestBuilder::CreateIrregularLakeAtRest(grid, 20, 20, 5.0f, updater.get());

        std::vector<float> initialWater = grid.GetLayer<float>("water_depth")->GetData();

        this->updater->Initialize(grid);

        // Execute 100 iterations
        for (int step = 0; step < 100; ++step) {
            this->updater->Step(grid);
        }

        const std::vector<float>& finalWater = grid.GetLayer<float>("water_depth")->GetData();

        for (size_t i = 0; i < initialWater.size(); ++i) {
            EXPECT_NEAR(initialWater[i], finalWater[i], 1e-5f)
                << "Instability detected on irregular terrain at cell " << i;
        }
    }

    /**
     * @brief Verifies that mass is not spontaneously generated.
     * * Ensures that a completely dry simulation domain remains at zero
     * water depth over time, proving the model does not "hallucinate" water.
     */
    TEST_P(StateUpdaterTest, DryTerrainRemainsDry) {
        app::core::grid::MapGrid grid;
        GridTestBuilder::CreateDryTerrain(grid, 20, 20, updater.get());

        this->updater->Initialize(grid);
        for (int step = 0; step < 50; ++step) {
            this->updater->Step(grid);
        }

        const std::vector<float>& finalWater = grid.GetLayer<float>("water_depth")->GetData();
        for (size_t i = 0; i < finalWater.size(); ++i) {
            EXPECT_LT(finalWater[i], 1e-6f)
                << "Hallucination detected: Water generated on dry terrain at cell " << i;
        }
    }

    /**
     * @brief Verifies that fluid does not climb up slopes without sufficient momentum.
     * * Tests a scenario where water rests at the bottom of an adverse slope.
     * The upper sections of the slope should remain strictly dry.
     */
    TEST_P(StateUpdaterTest, WetDryFrontOnAdverseSlope) {
        app::core::grid::MapGrid grid;
        GridIndexType cols = 20;
        GridTestBuilder::CreateAdverseSlope(grid, 20, cols, updater.get());

        this->updater->Initialize(grid);
        for (int step = 0; step < 50; ++step) {
            this->updater->Step(grid);
        }

        const std::vector<float>& finalWater = grid.GetLayer<float>("water_depth")->GetData();

        // Verify that the right half of the ramp (c >= cols/2) remains strictly dry
        for (size_t i = 0; i < finalWater.size(); ++i) {
            GridIndexType c = i % cols;
            if (c >= cols / 2) {
                EXPECT_LT(finalWater[i], 1e-5f)
                    << "Water climbed a slope without momentum at column " << c;
            }
        }
    }

    // =========================================================================
    // AUTOMATIC TEST SUITE REGISTRATION
    // =========================================================================

    // Instantiate the test suite with the dynamically discovered models
    INSTANTIATE_TEST_SUITE_P(
        FolderModels,                           // General suite name
        StateUpdaterTest,                       // The fixture class
        ::testing::ValuesIn(DiscoverModels()),  // Data provider function
        [](const ::testing::TestParamInfo<ModelTestParam>& info) {
            return info.param.testName;         // The sanitized name defined during discovery
        }
    );

} // namespace floodsim::tests
