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
    // BASE TESTS
    // =========================================================================

    /**
     * @brief Verifies that the model updater can initialize memory boundaries without crashing.
     *
     * Constructs a minimal 10x10 base grid and passes it to the generic Initialize
     * interface to ensure internal tensors allocate properly.
     */
    TEST_P(StateUpdaterTest, InitializationDoesNotCrash) {
        app::core::grid::MapGrid grid;
        GridTestBuilder::InitializeGridBase(grid, updater.get(), 10, 10);
        EXPECT_NO_THROW({ this->updater->Initialize(grid); });
    }

    // =========================================================================
    // PHYSICAL TESTS: HYDROSTATIC STABILITY & CONSERVATION
    // =========================================================================

    /**
     * @brief Validates the hydrostatic stability for a completely flat water surface.
     *
     * Simulates a standard resting lake environment for numerous steps. In the absence
     * of external forces, the initial depths should remain perfectly constant without
     * numerical drift or unprovoked oscillations.
     */
    TEST_P(StateUpdaterTest, FlatLakeRemainsAtRest) {
        app::core::grid::MapGrid grid;
        GridTestBuilder::CreateLakeAtRest(grid, 20, 20, 2.0f, updater.get());
        std::vector<float> initialWater = grid.GetLayer<float>("water_depth")->GetData();

        this->updater->Initialize(grid);

        for (int step = 0; step < 100; ++step) { this->updater->Step(grid); }

        const std::vector<float>& finalWater = grid.GetLayer<float>("water_depth")->GetData();
        for (size_t i = 0; i < initialWater.size(); ++i) {
            EXPECT_NEAR(initialWater[i], finalWater[i], 1e-4f) << "Instability detected in flat cell" << i;
        }
    }

    /**
     * @brief Validates hydrostatic stability over varying, irregular bottom topography.
     *
     * Ensures that a flat water surface remains motionless even if the underlying
     * bathymetry fluctuates. Tests if the solver correctly balances gravity with
     * hydrostatic pressure gradients.
     */
    TEST_P(StateUpdaterTest, IrregularLakeRemainsAtRest) {
        app::core::grid::MapGrid grid;
        GridTestBuilder::CreateIrregularLakeAtRest(grid, 20, 20, 5.0f, updater.get());
        std::vector<float> initialWater = grid.GetLayer<float>("water_depth")->GetData();

        this->updater->Initialize(grid);
        for (int step = 0; step < 100; ++step) { this->updater->Step(grid); }

        const std::vector<float>& finalWater = grid.GetLayer<float>("water_depth")->GetData();
        for (size_t i = 0; i < initialWater.size(); ++i) {
            EXPECT_NEAR(initialWater[i], finalWater[i], 1e-4f) << "Instability detected over irregular cell" << i;
        }
    }

    /**
     * @brief Verifies global mass conservation during dynamic fluid flow.
     *
     * Simulates a dam-break scenario and integrates the total fluid volume over the entire
     * domain using the relation $V = \sum_{i} h_i \Delta x^2$ before and after the temporal loop.
     * Asserts that the ML model does not artificially create or destroy mass beyond a strict tolerance.
     */
    TEST_P(StateUpdaterTest, MassIsConservedDuringMovement) {
        app::core::grid::MapGrid grid;
        GridTestBuilder::CreateDamBreakInPool(grid, 30, 30, updater.get());

        float cellSize = grid.GetMetadata().cell_size;
        float cellArea = cellSize * cellSize;

        // 1. Calculate actual initial volume
        const std::vector<float>& initialWater = grid.GetLayer<float>("water_depth")->GetData();
        float initialVolume = 0.0f;
        for (float h : initialWater) initialVolume += h * cellArea;

        this->updater->Initialize(grid);

        for (int step = 0; step < 20; ++step) { this->updater->Step(grid); }

        // 2. Calculate final volume
        const std::vector<float>& finalWater = grid.GetLayer<float>("water_depth")->GetData();
        float finalVolume = 0.0f;
        for (float h : finalWater) finalVolume += h * cellArea;

        float allowedError = initialVolume * 0.005f; // Tolerancia del 0.5%
        EXPECT_NEAR(initialVolume, finalVolume, allowedError)
            << "Mass conservation failed: Initial = " << initialVolume << " | Final =" << finalVolume;
    }

    /**
     * @brief Checks the system for spurious fluid generation on dry cells.
     *
     * Runs the model heavily on a 100% dry domain. Checks that the numerical scheme
     * or the underlying neural network does not "hallucinate" unphysical patches
     * of water out of nowhere.
     */
    TEST_P(StateUpdaterTest, DryTerrainRemainsDry) {
        app::core::grid::MapGrid grid;
        GridTestBuilder::CreateDryTerrain(grid, 20, 20, updater.get());

        this->updater->Initialize(grid);
        for (int step = 0; step < 50; ++step) { this->updater->Step(grid); }

        const std::vector<float>& finalWater = grid.GetLayer<float>("water_depth")->GetData();
        for (size_t i = 0; i < finalWater.size(); ++i) {
            EXPECT_LT(finalWater[i], 1e-6f) << "Spontaneous water generation on dry cell" << i;
        }
    }

    /**
     * @brief Evaluates the physical behavior of a wet-dry boundary against an incline.
     *
     * Ensures that resting fluid does not unrealistically climb an adverse slope
     * without sufficient kinetic energy, preventing unphysical spreading errors.
     */
    TEST_P(StateUpdaterTest, WetDryFrontOnAdverseSlope) {
        app::core::grid::MapGrid grid;
        GridIndexType cols = 20;
        GridTestBuilder::CreateAdverseSlope(grid, 20, cols, updater.get());

        this->updater->Initialize(grid);
        for (int step = 0; step < 50; ++step) { this->updater->Step(grid); }

        const std::vector<float>& finalWater = grid.GetLayer<float>("water_depth")->GetData();

        // Verify that the right half of the ramp (c >= cols/2) remains strictly dry
        for (size_t i = 0; i < finalWater.size(); ++i) {
            GridIndexType c = i % cols;
            if (c >= cols / 2) {
                EXPECT_LT(finalWater[i], 1e-5f) << "Water unrealistically climbed an energy gradient in column" << c;
            }
        }
    }

    // =========================================================================
    // PHYSICAL TESTS: ADVANCED DYNAMICS
    // =========================================================================

    /**
     * @brief Tests the spatial symmetry and isotropy of wave propagation.
     *
     * Simulates a single concentrated droplet released at the exact center of a square
     * grid. Asserts that the resulting wave travels symmetrically along all cardinal
     * and diagonal axes without bias or anisotropy generated by convolution artifacts.
     */
    TEST_P(StateUpdaterTest, WavePropagationIsSymmetric) {
        app::core::grid::MapGrid grid;
        GridIndexType size = 31;
        GridTestBuilder::CreateDropInCenter(grid, size, updater.get());

        this->updater->Initialize(grid);
        for (int step = 0; step < 10; ++step) { this->updater->Step(grid); }

        const std::vector<float>& finalWater = grid.GetLayer<float>("water_depth")->GetData();
        GridIndexType center = size / 2;

        for (GridIndexType dist = 1; dist < center - 1; ++dist) {
            // Cardinal axes
            float leftWater = finalWater[center * size + (center - dist)];
            float rightWater = finalWater[center * size + (center + dist)];
            EXPECT_NEAR(leftWater, rightWater, 1e-2f) << "Horizontal bias at distance" << dist;

            float topWater = finalWater[(center - dist) * size + center];
            float bottomWater = finalWater[(center + dist) * size + center];
            EXPECT_NEAR(topWater, bottomWater, 1e-2f) << "Vertical bias at distance" << dist;

            // Diagonals
            float topLeftWater = finalWater[(center - dist) * size + (center - dist)];
            float bottomRightWater = finalWater[(center + dist) * size + (center + dist)];
            EXPECT_NEAR(topLeftWater, bottomRightWater, 1e-2f) << "Diagonal bias at distance" << dist;
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
