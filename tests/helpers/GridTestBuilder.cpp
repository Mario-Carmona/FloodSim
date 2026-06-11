/**
 * @file GridTestBuilder.cpp
 * @brief Implementation of the scenario builder helpers for grid unit tests.
 */

#include "helpers/GridTestBuilder.hpp"

namespace floodsim::tests {

    void GridTestBuilder::InitializeGridBase(app::core::grid::MapGrid& grid,
                                             app::core::ports::StateUpdaterPort* updater,
                                             GridIndexType rows, GridIndexType cols) {
        // Configure physical scalar constraints for the test environment.
        std::unordered_map<std::string, std::string> scalars_config = {
            { "fluid_density", "1000.0" },  // Standard water density (kg/m³)
            { "fluid_viscosity", "0.001" }  // Dynamic water viscosity (Pa·s)
        };

        auto paramsInfo = updater->GetModelParamsInfo();

        // Inject flood risk layer metadata needed for test evaluation.
        paramsInfo.layers.push_back({
            .name = "flood_risk",
            .data_type = app::core::grid::DataType::kInt8,
            .load_required = false
            });

        // Bypass mandatory external file loading flags for a self-contained test.
        for (auto& layer : paramsInfo.layers) {
            layer.load_required = false;
        }
        for (auto& scalar : paramsInfo.scalars) {
            scalar.load_required = false;
        }

        // Enforce structural allocation on the MapGrid via its formal loader API.
        grid.Load(
            paramsInfo, nullptr, {}, scalars_config,
            std::chrono::duration<double>(0.01), std::chrono::system_clock::now()
        );

        grid.GetScalar<float>("delta_x")->SetValue(1.0f);

        size_t cell_count = static_cast<size_t>(rows * cols);

        grid.SetMetadata(app::core::grid::GridMetadata{
            .width = static_cast<int>(cols),
            .height = static_cast<int>(rows),
            .cell_count = static_cast<int>(cell_count),
            .cell_size = 1.0f
            });

        // Extract internal references to the allocated data layers.
        auto& elevationData = grid.GetLayer<float>("topo_bathy")->GetData();
        auto& waterData = grid.GetLayer<float>("water_depth")->GetData();
        auto& landCoverData = grid.GetLayer<int8_t>("land_cover")->GetData();
        auto& roughnessData = grid.GetLayer<float>("roughness")->GetData();
        auto& waterMovementData = grid.GetLayer<int8_t>("water_movement_state")->GetData();
        auto& floodRiskData = grid.GetLayer<int8_t>("flood_risk")->GetData();

        // Resize data vectors to match physical dimensions and assign defaults.
        elevationData.resize(cell_count, 0.0f);
        waterData.resize(cell_count, 0.0f);
        landCoverData.resize(cell_count, 0);
        roughnessData.resize(cell_count, 0.0f);
        waterMovementData.resize(cell_count, static_cast<int8_t>(app::core::ports::StateUpdaterPort::WaterMovementState::kDynamicState));
        floodRiskData.resize(cell_count, 0);
    }

    void GridTestBuilder::CreateLakeAtRest(app::core::grid::MapGrid& grid, GridIndexType rows, 
                                           GridIndexType cols, float water_depth, 
                                           app::core::ports::StateUpdaterPort* updater) {
        InitializeGridBase(grid, updater, rows, cols);
        size_t cell_count = static_cast<size_t>(rows * cols);
        auto& elevationData = grid.GetLayer<float>("topo_bathy")->GetData();
        auto& waterData = grid.GetLayer<float>("water_depth")->GetData();

        for (FlatVectorIndexType i = 0; i < cell_count; ++i) {
            GridIndexType r = i / cols;
            GridIndexType c = i % cols;

            // Smoothed perimeter walls set to exactly 1m above the water depth
            // to intentionally clamp extreme gradients.
            if (r == 0 || r == rows - 1 || c == 0 || c == cols - 1) {
                elevationData[i] = water_depth + 1.0f;
                waterData[i] = 0.0f;
            }
            else {
                elevationData[i] = 0.0f;
                waterData[i] = water_depth;
            }
        }
    }

    void GridTestBuilder::CreateIrregularLakeAtRest(app::core::grid::MapGrid& grid, GridIndexType rows, 
                                                    GridIndexType cols, float surface_elevation, 
                                                    app::core::ports::StateUpdaterPort* updater) {
        InitializeGridBase(grid, updater, rows, cols);
        size_t cell_count = static_cast<size_t>(rows * cols);
        auto& elevationData = grid.GetLayer<float>("topo_bathy")->GetData();
        auto& waterData = grid.GetLayer<float>("water_depth")->GetData();

        for (FlatVectorIndexType i = 0; i < cell_count; ++i) {
            GridIndexType r = i / cols;
            GridIndexType c = i % cols;

            if (r == 0 || r == rows - 1 || c == 0 || c == cols - 1) {
                elevationData[i] = surface_elevation + 1.0f;
                waterData[i] = 0.0f;
            }
            else {
                // Smooth bed variations (~0.5m amplitude) to avoid introducing extreme slopes.
                float bed = 1.0f + 0.5f * std::sin(static_cast<float>(r) * 0.5f) * std::cos(static_cast<float>(c) * 0.5f);
                elevationData[i] = bed;
                waterData[i] = std::max(0.0f, surface_elevation - bed);
            }
        }
    }

    void GridTestBuilder::CreateDamBreakInPool(app::core::grid::MapGrid& grid, GridIndexType rows, GridIndexType cols, 
                                               app::core::ports::StateUpdaterPort* updater) {
        InitializeGridBase(grid, updater, rows, cols);
        size_t cell_count = static_cast<size_t>(rows * cols);
        auto& elevationData = grid.GetLayer<float>("topo_bathy")->GetData();
        auto& waterData = grid.GetLayer<float>("water_depth")->GetData();

        for (FlatVectorIndexType i = 0; i < cell_count; ++i) {
            GridIndexType r = i / cols;
            GridIndexType c = i % cols;

            if (r == 0 || r == rows - 1 || c == 0 || c == cols - 1) {
                elevationData[i] = 4.0f;
                waterData[i] = 0.0f;
            }
            else {
                elevationData[i] = 0.0f;
                if (c >= cols / 2 - 2 && c <= cols / 2 + 1) {
                    waterData[i] = 0.2f;
                }
                else {
                    waterData[i] = 0.0f;
                }
            }
        }
    }

    void GridTestBuilder::CreateDropInCenter(app::core::grid::MapGrid& grid, GridIndexType size, 
                                             app::core::ports::StateUpdaterPort* updater) {
        InitializeGridBase(grid, updater, size, size);
        size_t cell_count = static_cast<size_t>(size * size);
        auto& elevationData = grid.GetLayer<float>("topo_bathy")->GetData();
        auto& waterData = grid.GetLayer<float>("water_depth")->GetData();

        GridIndexType center = size / 2;

        for (FlatVectorIndexType i = 0; i < cell_count; ++i) {
            GridIndexType r = i / size;
            GridIndexType c = i % size;

            if (r == 0 || r == size - 1 || c == 0 || c == size - 1) {
                elevationData[i] = 5.0f;
                waterData[i] = 0.0f;
            }
            else {
                elevationData[i] = 0.0f;
                if (r == center && c == center) {
                    waterData[i] = 0.2f;
                }
                else {
                    waterData[i] = 0.0f;
                }
            }
        }
    }

    void GridTestBuilder::CreateAdverseSlope(app::core::grid::MapGrid& grid, GridIndexType rows, GridIndexType cols, 
                                             app::core::ports::StateUpdaterPort* updater) {
        InitializeGridBase(grid, updater, rows, cols);
        size_t cell_count = static_cast<size_t>(rows * cols);
        auto& elevationData = grid.GetLayer<float>("topo_bathy")->GetData();
        auto& waterData = grid.GetLayer<float>("water_depth")->GetData();

        for (FlatVectorIndexType i = 0; i < cell_count; ++i) {
            GridIndexType r = i / cols;
            GridIndexType c = i % cols;

            if (r == 0 || r == rows - 1 || c == 0 || c == cols - 1) {
                elevationData[i] = 10.0f;
                waterData[i] = 0.0f;
            }
            else {
                elevationData[i] = static_cast<float>(c) * 0.1f;

                if (c < cols / 4) {
                    waterData[i] = std::max(0.0f, 1.5f - elevationData[i]);
                }
                else {
                    waterData[i] = 0.0f;
                }
            }
        }
    }

    void GridTestBuilder::CreateDryTerrain(app::core::grid::MapGrid& grid, GridIndexType rows, GridIndexType cols, 
                                           app::core::ports::StateUpdaterPort* updater) {
        InitializeGridBase(grid, updater, rows, cols);
        size_t cell_count = static_cast<size_t>(rows * cols);
        auto& elevationData = grid.GetLayer<float>("topo_bathy")->GetData();
        auto& waterData = grid.GetLayer<float>("water_depth")->GetData();

        for (FlatVectorIndexType i = 0; i < cell_count; ++i) {
            elevationData[i] = 1.0f;
            waterData[i] = 0.0f;
        }
    }

}  // namespace floodsim::tests