/**
 * @file ImageOutput.hpp
 * @brief Output port adapter for rendering and saving simulation snapshots as images.
 *
 * This adapter uses OpenCV to render water depth over a topographic background,
 * appending dynamic UI elements like time and color legends, and outputs PNG files.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include <opencv2/opencv.hpp>

#include "app/core/ports/OutputPort.hpp"

namespace floodsim::app::adapters::output {

/**
 * @class ImageOutput
 * @brief Renders simulation snapshots into 2D images and saves them to disk.
 */
class ImageOutput : public core::ports::OutputPort {
public:
    /**
     * @struct Config
     * @brief Configuration properties for rendering specific data layers.
     */
    struct Config {
        std::string units;  ///< Measurement units for the legend.
        float min_val;      ///< Fixed minimum value for consistent color mapping across frames.
        float max_val;      ///< Fixed maximum value for consistent color mapping.
        int colormap;       ///< OpenCV colormap identifier (e.g., cv::COLORMAP_JET).
        bool do_hillshade;  ///< Flag to enable terrain hillshading (for relief).
        bool mask_zero;     ///< Flag to mask zero values, rendering them transparent.
    };

    /**
     * @brief Default constructor for ImageOutput.
     */
    ImageOutput();

    ~ImageOutput() = default;

    /**
     * @brief Main execution loop for the image generation thread.
     *
     * Creates the output directory if necessary and continuously blocks waiting
     * for new snapshots to render until the manager is stopped.
     *
     * @param snapshot_manager Reference to the manager providing simulation snapshots.
     * @param output_path The root directory where output images will be saved.
     * @throws floodsim::app::exception::FloodSimException If the provided output_path is empty.
     */
    void Run(core::snapshot::SnapshotManager& snapshot_manager, const std::filesystem::path& output_path) override;

    /**
     * @brief Initializes the image output port with baseline grid data.
     *
     * Sets up map dimensions, extracts static topographical data, and calculates
     * baseline global minimum and maximum elevation values for rendering.
     *
     * @param grid The simulation map grid.
     * @param dataset_name The name of the dataset.
     * @param start_timestamp The initial time of the simulation.
     */
    void SetInitConfig(const core::grid::MapGrid& grid, const std::string& dataset_name,
        std::chrono::sys_seconds start_timestamp, const std::vector<config::FloodRiskLevel>& flood_risk_levels) override;

    /**
     * @brief Retrieves the identifier name for this thread.
     *
     * @return std::string The predefined thread name.
     */
    std::string GetThreadName() const override { return "Out_Image"; }

private:
    /**
     * @brief Saves a snapshot as a rendered PNG image.
     *
     * @param snapshot The simulation snapshot to render.
     * @param changes List of state changes (required by snapshot data structure).
     * @param images_output_path Directory to save the resulting image.
     */
    void SaveSnapshotAsImage(const core::snapshot::Snapshot& snapshot, const core::snapshot::ChangeList& changes,
        const std::filesystem::path& images_output_path);

    /**
     * @brief Generates the static terrain background.
     *
     * Optionally applies a topographic color map and hillshading effect.
     *
     * @param snapshot The simulation snapshot (for context).
     * @param use_colormap Whether to apply a topographic color map instead of a flat maquette style.
     * @return cv::Mat The generated background image matrix.
     */
    cv::Mat CreateTerrainBackground(const core::snapshot::Snapshot& snapshot, bool use_colormap);

    /**
     * @brief Draws user interface elements (titles, legends) onto the canvas.
     *
     * @param canvas The target image matrix to draw on.
     * @param snapshot The current snapshot (used to display time).
     * @param base_scale Base scaling factor for UI elements to adapt to image resolution.
     * @param thickness Line thickness for text rendering.
     * @param margin_title Top margin allocated for the title header.
     * @param sidebar_width Right margin allocated for color legends.
     */
    void DrawUI(cv::Mat& canvas, const core::snapshot::Snapshot& snapshot, double base_scale,
        int thickness, int margin_title, int sidebar_width);
        
    int rows_ = 0;
    int cols_ = 0;
    const float* topo_bathy_ = nullptr;

    std::unordered_map<std::string, Config> layer_configs_;
};

} // namespace floodsim::app::adapters::output
