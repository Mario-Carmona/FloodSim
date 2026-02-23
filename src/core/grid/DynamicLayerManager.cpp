/**
 * @file DynamicLayerManager.cpp
 * @brief Implementation of HDF5 streaming and spatial resampling logic.
 */

#include "core/grid/DynamicLayerManager.hpp"

#include <cmath>
#include <fmt/core.h>
#include <algorithm>
#include <stdexcept>

#include "logging/Logger.hpp"

namespace danasim {

    void DynamicLayerManager::initLayer(MapGrid& grid, const std::filesystem::path& filePath, float timeStep, LayerId layerId) {
        try {
            LOG_INFO("Initializing Streamed Layer Manager for '{}' from: {}", magic_enum::enum_name(layerId), filePath.string());

            // Open file in Read-Only mode
            HighFive::File file(filePath.string(), HighFive::File::ReadOnly);

            // 1. Specific initialization based on LayerId logic
            switch (layerId) {
            case LayerId::Rainfall: {
                initRainfallLayer(file, layerId);
                break;
            }
            default:
                auto msg = fmt::format(
                    "No specific initialization logic for streamed layer: {}", magic_enum::enum_name(layerId)
                );
                LOG_ERROR("{}", msg);
                throw std::runtime_error(msg);
                break;
            }

            // 2. Retrieve Dimensions
            // We expect 3D data: [Time, Y, X]
            auto dims = dataset_[layerId].getSpace().getDimensions();
            if (dims.size() != 3) {
                std::string msg = "Layer Data must be 3D [Time, Y, X]";
                LOG_ERROR("{}", msg);
                throw std::runtime_error(msg);
            }

            layerDimTime_[layerId] = dims[0]; // Times
            layerRows_[layerId] = dims[1];    // Rows (Height)
            layerCols_[layerId] = dims[2];    // Cols (Width)

            if (downgradeFactor_[layerId] > 1) {
                // 3. Build the Index Map
                initLayerBuffer(grid, layerId);
            }

            // 4. Perform an initial update to set t=0
            updateLayer(grid, 0, timeStep, layerId);
        }
        catch (const std::exception& e) {
            LOG_ERROR("Failed to init streamed layer {}: {}", magic_enum::enum_name(layerId), e.what());
            throw;
        }
    }

    void DynamicLayerManager::initRainfallLayer(HighFive::File& file, LayerId layerId) {
        // 1. Access the "Data" dataset
        dataset_[layerId] = HighFive::DataSet(file.getDataSet("data"));

        // 2. Read Group Attributes
        file.getAttribute("downgrade_factor").read(downgradeFactor_[layerId]);
        file.getAttribute("time_step_seconds").read(h5TimeStepSeconds_[layerId]);
    }

    void DynamicLayerManager::initLayerBuffer(const MapGrid& grid, LayerId layerId) {
        // 1. Resize the Read Buffer
        // This buffer holds the raw data from one single HDF5 frame.
        readBuffer_[layerId].resize(layerRows_[layerId] * layerCols_[layerId]);


        // --------------------------------------------------------
        // 2. PRE-CALCULATE SPATIAL MAPPING (Resampling Setup)
        // --------------------------------------------------------

        // Cache dimensions to avoid map lookups inside the loop
        GridIndexType h5Rows = layerRows_[layerId];
        GridIndexType h5Cols = layerCols_[layerId];
        GridIndexType factor = downgradeFactor_[layerId];

        // Get a direct reference to the target index vector
        std::vector<FlatVectorIndexType>& indexMap = indexMap_[layerId];
        std::vector<bool>& insideMap = insideMap_[layerId];

        // Resize map to match the Simulation Grid (Total Cells)
        // Initialize with -1 to mark cells outside the HDF5 coverage area.
        indexMap.resize(grid.cellCount(), 0);
        insideMap.resize(grid.cellCount(), false);

        // Iterate over every cell in the Simulation Grid
        for (GridIndexType r = 0; r < grid.rows(); ++r) {
            for (GridIndexType c = 0; c < grid.cols(); ++c) {
                // Map Sim(r,c) -> HDF5(hr, hc) using integer division (Downscaling)
                GridIndexType hr = r / factor;
                GridIndexType hc = c / factor;

                // Boundary Check:
                // Ensure the calculated HDF5 coordinate is valid.
                if (hr < h5Rows && hc < h5Cols) {
                    // Store the HDF5 index in the map
                    FlatVectorIndexType idx = r * grid.cols() + c;
                    indexMap[idx] = hr * h5Cols + hc;
                    insideMap[idx] = true;
                }
            }
        }
    }

    void DynamicLayerManager::updateAllLayers(MapGrid& grid, StepType currentStep, float timeStep) {
        // 1. Calculate absolute simulation time once
        float simTime = currentStep * timeStep;
        
        // 2. Iterate ONLY over registered streamed layers
        for (auto& value : magic_enum::enum_values<LayerId>()) {
            const auto& desc = MapGrid::getDescriptor(value);

            if (desc.source == LayerSource::Dynamic) {
                // 3. Calculate the frame index from HDF5
                size_t frameIdx = static_cast<size_t>(simTime / h5TimeStepSeconds_[value]);

                // 4. Update only if the frame has changed
                if (currentFrameIdx_[value] < frameIdx) {
                    // Perform I/O and grid projection
                    updateLayer(grid, frameIdx, timeStep, value);
                }
            }
        }
    }

    void DynamicLayerManager::updateLayer(MapGrid& grid, size_t frameIdx, float timeStep, LayerId layerId) {
        try {
            // Update state tracker
            currentFrameIdx_[layerId] = frameIdx;

            // --------------------------------------------------------
            // 1. BOUNDARY CHECK (End of Data)
            // --------------------------------------------------------
            // If the simulation time exceeds the data available in the HDF5 file,
            // we default to a "neutral" state (e.g., 0.0f for Rainfall).
            if (frameIdx >= layerDimTime_[layerId]) {
                // Get raw pointer for fast block filling
                float* layerPtr = grid.getLayer<float>(layerId).data();
                std::fill(layerPtr, layerPtr + grid.cellCount(), 0.0f);
                return;
            }

            // --------------------------------------------------------
            // 2. SETUP HDF5 HYPERSLAB (Selection)
            // --------------------------------------------------------
            // We select a single time slice from the 3D dataset [Time, Rows, Cols].
            // Offset: {Time=frameIdx, Y=0, X=0}
            // Count:  {1, Rows, Cols}
            std::vector<size_t> offset = { frameIdx, 0, 0 };
            std::vector<size_t> count = { 1, static_cast<size_t>(layerRows_[layerId]), static_cast<size_t>(layerCols_[layerId]) };

            // Select the file region
            HighFive::Selection selection = dataset_[layerId].select(offset, count);

            // --------------------------------------------------------
            // 3. DEFINE MEMORY SPACE
            // --------------------------------------------------------
            // We must tell HDF5 that our destination memory is shaped like the slice {1, Rows, Cols},
            // even though it is physically a flat 1D array in C++.
            HighFive::DataSpace memSpace(count);

            // --------------------------------------------------------
            // 4. READ OPERATION (Branching)
            // --------------------------------------------------------

            if (downgradeFactor_[layerId] == 1) {
                // === SCENARIO A: Direct Write ===
                // The resolutions match. We write directly to the Grid's memory.

                float* destPtr = grid.getLayer<float>(layerId).data();

                // Use low-level C API (H5Dread) to bypass C++ vector reallocation.
                // We write straight into 'destPtr'.
                herr_t status = H5Dread(
                    dataset_[layerId].getId(),      // Dataset ID
                    H5T_NATIVE_FLOAT,               // Memory Data Type
                    memSpace.getId(),               // Memory Space ID
                    selection.getSpace().getId(),   // File Space ID (with Hyperslab)
                    H5P_DEFAULT,                    // Transfer Properties
                    destPtr                         // Destination Pointer
                );

                if (status < 0) {
                    std::string msg = "H5Dread failed directly to grid";
                    LOG_ERROR("{}", msg);
                    throw std::runtime_error(msg);
                }
            }
            else {
                // === SCENARIO B: Buffered Read & Resampling ===
                // The resolutions differ. Read to a small buffer, then scatter to the grid.

                // 4.1 Read into the small intermediate buffer
                float* bufferPtr = readBuffer_[layerId].data();

                herr_t status = H5Dread(
                    dataset_[layerId].getId(),
                    H5T_NATIVE_FLOAT,
                    memSpace.getId(),
                    selection.getSpace().getId(),
                    H5P_DEFAULT,
                    bufferPtr
                );

                if (status < 0) {
                    std::string msg = "H5Dread failed to buffer";
                    LOG_ERROR("{}", msg);
                    throw std::runtime_error(msg);
                }

                // 4.2 Spatial Mapping (Scatter)
                // Access optimization: Local references
                std::vector<float>& buffer = readBuffer_[layerId];
                std::vector<FlatVectorIndexType>& indexMap = indexMap_[layerId];
                std::vector<bool>& insideMap = insideMap_[layerId];
                std::vector<float>& layer = grid.getLayer<float>(layerId);

                // Iterate over the Simulation Grid (Fine resolution)
                for (FlatVectorIndexType i = 0; i < grid.cellCount(); ++i) {
                    if (insideMap[i]) {
                        // Map the value from the Coarse Buffer -> Fine Cell
                        // Apply unit conversion (e.g., mm/hr -> meters/step)
                        layer[i] = calculateLayerValue(buffer[indexMap[i]], timeStep, layerId);
                    }
                    else {
                        // Cell is outside the HDF5 coverage area
                        layer[i] = 0.0f;
                    }
                }
            }
        }
        catch (const HighFive::Exception& e) {
            LOG_ERROR("Error reading rain frame {}: {}", frameIdx, e.what());
        }
    }

    float DynamicLayerManager::calculateLayerValue(float bufferValue, float timeStep, LayerId layerId) {
        switch (layerId) {

        case LayerId::Rainfall: {
            // ---------------------------------------------------------
            // LOGIC: Rate-to-Accumulation Conversion + Unit Conversion
            // ---------------------------------------------------------
            // Input:  Intensity Rate in [mm/hour]
            // Output: Absolute depth added in [meters] per step
            //
            // Formula breakdown:
            // 1. mm -> m:      val / 1000.0
            // 2. hour -> sec:  val / 3600.0
            // 3. Rate -> Step: val * timeStep
            //
            // Combined: val * timeStep / 3,600,000.0

            constexpr float MM_TO_M_FACTOR = 0.001f;
            constexpr float HOUR_TO_SEC_INV = 1.0f / 3600.0f;

            return (bufferValue * MM_TO_M_FACTOR) * (timeStep * HOUR_TO_SEC_INV);
            break;
        }

        default:
            auto msg = fmt::format(
                "calculateLayerValue: Conversion logic not defined for layer ID {}",
                static_cast<int>(layerId)
            );
            LOG_ERROR("{}", msg);
            throw std::runtime_error(msg);
            break;
        }
    }

} // namespace danasim
