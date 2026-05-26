/**
 * @file ImageOutput.cpp
 * @brief Implementation of the ImageOutput class for rendering simulation data via OpenCV.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/adapters/output/ImageOutput.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include <fmt/chrono.h>
#include <fmt/core.h>

#include "app/config/Config.hpp"
#include "app/core/grid/MapGrid.hpp"
#include "logging/Logger.hpp"

namespace floodsim {

namespace {

/**
 * @brief Helper function to create a geographic color lookup table.
 *
 * Defines a custom palette transitioning through Dark Green -> Light Green -> Brown -> White.
 *
 * @return cv::Mat A 1x256 8UC3 matrix representing the Look-Up Table (LUT).
 */
cv::Mat GetTopographicLUT() {
    cv::Mat lut(1, 256, CV_8UC3);
    for (int i = 0; i < 256; ++i) {
        cv::Vec3b color;
        // Define 4 key points (BGR format):
        // 0:   Dark Green  (30, 100, 30)
        // 120: Light Green (80, 180, 100)
        // 190: Brown       (60, 100, 160)
        // 255: White       (255, 255, 255)

        if (i < 120) {
            // Dark Green to Light Green (Valleys)
            float t = static_cast<float>(i) / 120.0f;
            color[0] = static_cast<uchar>(30 + t * (80 - 30));    // Blue
            color[1] = static_cast<uchar>(100 + t * (180 - 100)); // Green
            color[2] = static_cast<uchar>(30 + t * (100 - 30));   // Red
        }
        else if (i < 190) {
            // Light Green to Brown (Mid-level mountains)
            float t = static_cast<float>(i - 120) / 70.0f;
            color[0] = static_cast<uchar>(80 - t * (80 - 60));    // Blue
            color[1] = static_cast<uchar>(180 - t * (180 - 100)); // Green
            color[2] = static_cast<uchar>(100 + t * (160 - 100)); // Red
        }
        else {
            // Brown to White (Peaks / Snow)
            float t = static_cast<float>(i - 190) / 65.0f;
            color[0] = static_cast<uchar>(60 + t * (255 - 60));
            color[1] = static_cast<uchar>(100 + t * (255 - 100));
            color[2] = static_cast<uchar>(160 + t * (255 - 160));
        }
        lut.at<cv::Vec3b>(i) = color;
    }
    return lut;
}

}  // namespace

ImageOutput::ImageOutput() {
    layer_configs_["topo_bathy"] = Config{
        .units = "Elevation (m)",
        .min_val = 0.0f,
        .max_val = 1000.0f,  // Placeholder, dynamically calculated later
        .colormap = cv::COLORMAP_BONE,
        .do_hillshade = true,
        .mask_zero = false };

    layer_configs_["water_depth"] = Config{
        .units = "Water Depth (m)",
        .min_val = 0.0f,
        .max_val = 10.0f,
        .colormap = cv::COLORMAP_JET,
        .do_hillshade = false,
        .mask_zero = true };
}

void ImageOutput::Run(SnapshotManager& snapshot_manager, const std::filesystem::path& output_path) {
    if (output_path.empty()) {
        LOG_ERROR("Provided output path for images is empty.");
        throw std::invalid_argument("ImageOutput: output_path cannot be empty.");
    }

    std::filesystem::path images_output_path = output_path / "images";

    if (!std::filesystem::exists(images_output_path)) {
        std::filesystem::create_directories(images_output_path);
    }

    std::chrono::system_clock::time_point last_processed_step = std::chrono::system_clock::time_point::min();

    while (true) {
        try {
            // 1. Block until receiving data
            auto [data, guard] = snapshot_manager.WaitForSnapshot(last_processed_step);
            auto [snapshot, changes] = data;

            // If guard is null, the manager has stopped
            if (!guard) {
                LOG_INFO("Stopping ImageOutput thread (manager stopped).");
                break;
            }

            LOG_INFO("Consuming snapshot for image rendering.");

            // 2. Processing
            SaveSnapshotAsImage(snapshot, changes, images_output_path);

            // Update tracking time
            last_processed_step = snapshot.Time();
        }
        catch (const std::exception& ex) {
            LOG_ERROR("Error in ImageOutput loop: {}", ex.what());
            // Loop continues; guard releases Core automatically.
        }
    }
}

void ImageOutput::SetInitConfig(const MapGrid& grid, const std::string& /* dataset_name */,
        std::chrono::sys_seconds /* start_timestamp */) {

    rows_ = static_cast<int>(grid.GetRows());
    cols_ = static_cast<int>(grid.GetCols());

    topo_bathy_ = grid.GetLayer<float>("topo_bathy")->GetData().data();

    const auto& elev_data = grid.GetLayer<float>("topo_bathy")->GetData();
    layer_configs_["topo_bathy"].min_val = *std::min_element(elev_data.begin(), elev_data.end());
    layer_configs_["topo_bathy"].max_val = *std::max_element(elev_data.begin(), elev_data.end());
}

cv::Mat ImageOutput::CreateTerrainBackground(const Snapshot& /* snapshot */, bool use_colormap) {
    // Wrap data in Mat (no initial copy)
    cv::Mat raw_elev(rows_, cols_, CV_32F, const_cast<float*>(topo_bathy_));

    const auto& cfg = layer_configs_["topo_bathy"];

    // 1. Normalize Elevation (0-255) based on fixed min/max limits
    cv::Mat norm_elev;
    cv::Mat diff = raw_elev - cfg.min_val;
    diff.convertTo(norm_elev, CV_8U, 255.0 / (cfg.max_val - cfg.min_val));

    // 2. Apply Colormap
    cv::Mat base_terrain;

    if (use_colormap) {
        // cv::LUT requires a 3-channel input matching the LUT.
        // Convert grayscale 1-channel to BGR 3-channels.
        cv::Mat norm_elev_3ch;
        cv::cvtColor(norm_elev, norm_elev_3ch, cv::COLOR_GRAY2BGR);

        static cv::Mat topo_lut = GetTopographicLUT();
        cv::LUT(norm_elev_3ch, topo_lut, base_terrain);
    }
    else {
        // Maquette Style (Light Gray) - Ideal for overlaying fluid data without color distortion
        base_terrain = cv::Mat(rows_, cols_, CV_8UC3, cv::Scalar(240, 240, 240));
    }

    // 3. Compute Hillshade
    if (cfg.do_hillshade) {
        cv::Mat grad_x, grad_y;
        cv::Sobel(norm_elev, grad_x, CV_32F, 1, 0, 3);
        cv::Sobel(norm_elev, grad_y, CV_32F, 0, 1, 3);

        cv::Mat magnitude = cv::abs(grad_x) + cv::abs(grad_y);
        cv::Mat hillshade;
        cv::normalize(magnitude, hillshade, 0, 255, cv::NORM_MINMAX, CV_8U);
        cv::bitwise_not(hillshade, hillshade);  // Invert: Slope = Darker

        cv::Mat hillshade_bgr;
        cv::cvtColor(hillshade, hillshade_bgr, cv::COLOR_GRAY2BGR);

        // Blend Color * Shadow
        cv::multiply(base_terrain, hillshade_bgr, base_terrain, 1.0 / 255.0);
    }

    return base_terrain;
}

void ImageOutput::DrawUI(cv::Mat& canvas, const Snapshot& snapshot, double base_scale,
        int thickness, int margin_title, int sidebar_width) {

    int rows = canvas.rows - margin_title;
    int cols = canvas.cols - sidebar_width;

    // A. TITLE
    std::string title = fmt::format("Time: {:%FT%T}", snapshot.Time());
    int font_face = cv::FONT_HERSHEY_DUPLEX;
    double font_scale = base_scale * 1.2;
    cv::Size text_size = cv::getTextSize(title, font_face, font_scale, thickness, nullptr);

    cv::putText(canvas, title,
        cv::Point((canvas.cols - text_size.width) / 2, (margin_title + text_size.height) / 2),
        font_face, font_scale, cv::Scalar(0, 0, 0), thickness, cv::LINE_AA);

    // B. SIDEBAR LEGENDS
    int half_side = sidebar_width / 2;
    int side_h = rows;
    int side_y = margin_title;
    int start_x = cols;

    // Lambda to draw a single color bar
    auto paint_bar = [&](int offset_x, const Config& cfg, const std::string& label) {
        int bar_w = static_cast<int>(half_side * 0.4);
        int bar_h = static_cast<int>(side_h * 0.65);
        int x_pos = start_x + offset_x + (half_side - bar_w) / 2;
        int y_pos = side_y + (side_h - bar_h) / 2;

        int text_padding = static_cast<int>(40 * base_scale);

        // Top label
        cv::Size sz = cv::getTextSize(label, font_face, base_scale, thickness, nullptr);
        cv::putText(canvas, label, { x_pos + bar_w / 2 - sz.width / 2, y_pos - text_padding },
            font_face, base_scale, { 0, 0, 0 }, thickness, cv::LINE_AA);

        // Generate gradient
        cv::Mat grad(256, 1, CV_8U);
        for (int i = 0; i < 256; ++i) {
            grad.at<uint8_t>(i) = static_cast<uint8_t>(255 - i);
        }

        cv::Mat color_bar;
        cv::applyColorMap(grad, color_bar, cfg.colormap);

        cv::resize(color_bar, color_bar, cv::Size(bar_w, bar_h));
        color_bar.copyTo(canvas(cv::Rect(x_pos, y_pos, bar_w, bar_h)));

        // Inner lambda to paint min/max/mid values
        auto paint_bar_label = [](cv::Mat& canvas, float val, int point_x, int point_y,
            double scale, int font_face, int thickness) {
                cv::putText(canvas, fmt::format("{:.1f}m", val), { point_x, point_y },
                    font_face, scale, { 50, 50, 50 }, thickness, cv::LINE_AA);
            };

        // Side Values Min/Max
        double label_scale = base_scale * 0.55;
        int font_height = static_cast<int>(10 * base_scale);
        int text_x = x_pos + bar_w + 10;
        float mid_val = (cfg.min_val + cfg.max_val) / 2.0f;

        paint_bar_label(canvas, cfg.max_val, text_x, y_pos + font_height, label_scale, font_face, thickness);
        paint_bar_label(canvas, mid_val, text_x, y_pos + bar_h / 2 + font_height / 2, label_scale, font_face, thickness);
        paint_bar_label(canvas, cfg.min_val, text_x, y_pos + bar_h, label_scale, font_face, thickness);
    };

    paint_bar(0, layer_configs_["topo_bathy"], "TopoBathy");
    paint_bar(half_side, layer_configs_["water_depth"], "Water");
}

void ImageOutput::SaveSnapshotAsImage(const Snapshot& snapshot, const ChangeList& /* changes */,
        const std::filesystem::path& images_output_path) {

    // 1. Dynamic UI Scaling (Adapts font size based on image width)
    double base_scale = std::max(0.6f, cols_ / 1000.0f);
    int thickness = std::max(1, static_cast<int>(base_scale * 2));
    int margin_title = static_cast<int>(60 * base_scale);
    int sidebar_width = static_cast<int>(350 * base_scale);

    // ---------------------------------------------------------------------
    // STEP 1: Generate Base Terrain
    // ---------------------------------------------------------------------
    cv::Mat maquette_terrain = CreateTerrainBackground(snapshot, false);
    cv::Mat combined_img = maquette_terrain.clone();

    // ---------------------------------------------------------------------
    // STEP 2: Render Water Overlay
    // ---------------------------------------------------------------------
    layer_configs_["water_depth"].max_val =
        *std::max_element(snapshot.WaterDepth().begin(), snapshot.WaterDepth().end());

    const auto& w_cfg = layer_configs_["water_depth"];

    cv::Mat raw_water(rows_, cols_, CV_32F, const_cast<float*>(snapshot.WaterDepth().data()));
    cv::Mat norm_water;

    raw_water.convertTo(norm_water, CV_8U, 255.0 / w_cfg.max_val);

    cv::Mat water_color_map;
    cv::applyColorMap(norm_water, water_color_map, w_cfg.colormap);

    const int8_t* flood_risk_ptr = snapshot.FloodRisk().data();

    // Efficient pixel traversal for Alpha Blending
    for (int r = 0; r < rows_; ++r) {
        cv::Vec3b* dst_ptr = combined_img.ptr<cv::Vec3b>(r);
        const cv::Vec3b* src_water_ptr = water_color_map.ptr<cv::Vec3b>(r);
        const int8_t* row_flood_risk_ptr = flood_risk_ptr + (r * cols_);

        for (int c = 0; c < cols_; ++c) {
            if (row_flood_risk_ptr[c] > 0) {
                // Alpha Blend: 85% Water + 15% Terrain
                cv::Vec3b w_col = src_water_ptr[c];
                cv::Vec3b t_col = dst_ptr[c];

                dst_ptr[c] = cv::Vec3b(
                    static_cast<uchar>(w_col[0] * 0.85 + t_col[0] * 0.15),
                    static_cast<uchar>(w_col[1] * 0.85 + t_col[1] * 0.15),
                    static_cast<uchar>(w_col[2] * 0.85 + t_col[2] * 0.15)
                );
            }
        }
    }

    // ---------------------------------------------------------------------
    // STEP 3: Create Final Canvas and Apply UI
    // ---------------------------------------------------------------------
    cv::Mat final_canvas(rows_ + margin_title, cols_ + sidebar_width, CV_8UC3, cv::Scalar(250, 250, 250));
    combined_img.copyTo(final_canvas(cv::Rect(0, margin_title, cols_, rows_)));

    DrawUI(final_canvas, snapshot, base_scale, thickness, margin_title, sidebar_width);

    // ---------------------------------------------------------------------
    // STEP 4: Save Image
    // ---------------------------------------------------------------------
    std::string filename = fmt::format("Combined_{:%FT%T}.png", snapshot.Time());
    std::replace(filename.begin(), filename.end(), ':', '-');

    if (cv::imwrite((images_output_path / filename).string(), final_canvas)) {
        LOG_INFO("Saved image: {}", filename);
    }
    else {
        LOG_ERROR("Failed to write image: {}", (images_output_path / filename).string());
    }
}

} // namespace floodsim
