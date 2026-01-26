/**
 * @file StreamedLayerManager.hpp
 * @brief Manages temporal data layers loaded from HDF5 files.
 *
 * Handles the logic for mapping large-scale environmental data (like rainfall)
 * onto the finer simulation grid, updating it step-by-step.
 *
 * @version 1.0
 * @date 2026
 * @copyright Copyright (c) 2026 Danasim
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

 // HighFive for HDF5 interaction
#include <highfive/H5File.hpp>

#include "core/grid/MapGrid.hpp"

namespace danasim {

    /**
     * @class StreamedLayerManager
     * @brief Controller for time-variant HDF5 layers.
     */
    class StreamedLayerManager {
    public:
        StreamedLayerManager() = default;
        ~StreamedLayerManager() = default;

        /**
         * @brief Initializes a specific layer from an HDF5 file.
         *
         * Opens the file, reads dimensions, and pre-calculates the spatial mapping index
         * (lookup table) to optimize updates during the simulation.
         *
         * @param grid Reference to the main simulation grid.
         * @param filePath Path to the .h5 file.
         * @param timeStep The simulation time step (dt) in seconds.
         * @param layerId The identifier of the layer to initialize (e.g., Rainfall).
         * @throw std::runtime_error If dimensions are invalid or file is missing.
         */
        void initLayer(MapGrid& grid, const std::filesystem::path& filePath, float timeStep, LayerId layerId);

        /**
         * @brief Updates all registered streamed layers for the current simulation step.
         *
         * Checks if the simulation time requires a new frame from the HDF5 file
         * and updates the grid accordingly.
         *
         * @param grid The mutable simulation grid.
         * @param currentStep The current simulation iteration count.
         * @param timeStep The simulation time step (dt).
         */
        void updateAllLayers(MapGrid& grid, StepType currentStep, float timeStep);

    private:
        /**
         * @brief Pre-calculates the spatial mapping between HDF5 pixels and Grid cells.
         * * Generates the `indexMap_` which allows O(1) projection during the update loop.
         */
        void initLayerBuffer(const MapGrid& grid, LayerId layerId);

        /**
         * @brief Validates and prepares the HDF5 dataset specifically for Rainfall.
         */
        void initRainfallLayer(HighFive::File& file, LayerId layerId);

        /**
         * @brief Reads a specific time frame from disk and applies it to the grid.
         * * @param frameIdx The index of the time slice in the HDF5 dataset (0-based).
         */
        void updateLayer(MapGrid& grid, size_t frameIdx, float timeStep, LayerId layerId);

        /**
         * @brief Transforms a raw value from the source buffer into the specific unit required for a single simulation step.
         *
         * This function handles **unit conversion** and **time-scaling**.
         *
         * @param bufferValue The raw value read from the HDF5 dataset.
         * @param timeStep The current simulation time step (dt) in seconds.
         * @param layerId The identifier of the layer being processed.
         * @return float The calculated increment in standard simulation units (usually meters).
         */
        [[nodiscard]]
        float calculateLayerValue(float bufferValue, float timeStep, LayerId layerId);

        // --- Data Handling ---

        // Open file handles/datasets
        std::unordered_map<LayerId, HighFive::DataSet> dataset_;

        // Ratio between Simulation Grid resolution and HDF5 resolution
        std::unordered_map<LayerId, uint32_t> downgradeFactor_;

        // HDF5 Dataset Dimensions
        std::unordered_map<LayerId, size_t> layerDimTime_; // Dimension 0 (Time)
        std::unordered_map<LayerId, GridIndexType> layerRows_;    // Dimension 1 (Y)
        std::unordered_map<LayerId, GridIndexType> layerCols_;    // Dimension 2 (X)

        // State Tracking
        std::unordered_map<LayerId, size_t> currentFrameIdx_;
        // Time step of the HDF5 file itself (e.g., data every 3600 seconds)
        std::unordered_map<LayerId, float> h5TimeStepSeconds_;

        // --- Buffers ---

        // 1. Raw buffer: Holds the small frame read from HDF5
        // Optimization: Re-used to avoid allocation every frame.
        std::unordered_map<LayerId, std::vector<float>> readBuffer_;

        // 2. Index Map: Lookup table [GridCellIndex -> HDF5BufferIndex]
        std::unordered_map<LayerId, std::vector<FlatVectorIndexType>> indexMap_;

        std::unordered_map<LayerId, std::vector<bool>> insideMap_;
    };

} // namespace danasim
