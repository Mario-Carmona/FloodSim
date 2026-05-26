/**
 * @file InputTab.cpp
 * @brief Implementation of the Input Data configuration tab.
 *
 * Provides the GUI layout to configure input sources for the simulation,
 * including static topography, dynamic boundary conditions, and auto-extracted
 * scalars from the selected AI model's metadata.
 * 
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "gui/tabs/Tabs.hpp"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_set>
#include <variant>

#include <imgui.h>
#include <nlohmann/json.hpp>

#include "app/io/formats/file/FileDynamicFormat.hpp"
#include "app/io/formats/file/FileStaticFormat.hpp"

namespace {

/**
    * @brief C++20 Visitor Helper for std::visit.
    */
template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

/**
    * @brief Extracts scalar configuration parameters from a model's JSON metadata.
    * * Inspects the currently selected AI model path (e.g., ONNX model folder),
    * looks for a 'metadata.json', and auto-populates the required scalar inputs.
    * * @param updater_config Read-only reference to the state updater configuration.
    * @param input_config The input configuration to modify with discovered scalars.
    * @param last_loaded_model_path Tracks the last processed model to avoid redundant disk I/O per frame.
    */
void ExtractScalars(const floodsim::UpdaterConfig& updater_config,
                    floodsim::InputConfig& input_config,
                    std::filesystem::path& last_loaded_model_path) {

    std::visit(Overloaded{
        [&](const floodsim::OnnxUpdaterConfig& onnx_cfg) {
            // Return immediately if the model path hasn't changed to avoid disk I/O on every UI frame
            if (onnx_cfg.model_path.empty() || onnx_cfg.model_path == last_loaded_model_path) {
                return;
            }

            std::filesystem::path metadata_path = onnx_cfg.model_path / "metadata.json";

            if (std::filesystem::exists(metadata_path)) {
                try {
                    std::ifstream file(metadata_path);
                    nlohmann::json metadata = nlohmann::json::parse(file);

                    std::unordered_set<std::string> expected_scalars;

                    for (const char* method : { "preprocess", "step" }) {
                        for (const auto& input : metadata[method]["inputs"]) {
                            if (input["shape"].empty()) {
                                std::string scalar_name = input["name"];
                                expected_scalars.insert(scalar_name);
                            }
                        }
                    }

                    for (const char* implicit_scalar : { "delta_x", "delta_t" }) {
                        expected_scalars.erase(implicit_scalar);
                    }

                    // Remove scalars that are no longer expected by the new model
                    for (auto it = input_config.scalars.begin(); it != input_config.scalars.end(); ) {
                        if (expected_scalars.find(it->first) == expected_scalars.end()) {
                            it = input_config.scalars.erase(it);
                        }
                        else {
                            ++it;
                        }
                    }

                    // Add new expected scalars
                    for (const auto& scalar : expected_scalars) {
                        if (input_config.scalars.find(scalar) == input_config.scalars.end()) {
                            input_config.scalars[scalar] = "";
                        }
                    }
                }
                catch (const nlohmann::json::exception& e) {
                    std::cerr << "[Warning] Failed to parse model metadata JSON: " << e.what() << "\n";
                }
                catch (const std::exception& e) {
                    std::cerr << "[Warning] Unexpected error reading model metadata: " << e.what() << "\n";
                }
            }

            // Update the tracker so we don't process this path again until it changes
            last_loaded_model_path = onnx_cfg.model_path;
        },
        [](auto&&) {
            // Fallback for other updater types without scalar extraction logic
        }
        }, 
        updater_config
    );
}

} // anonymous namespace

namespace floodsim::gui {

void RenderInputTab(InputConfig& input_config, const UpdaterConfig& updater_config) {
    // Static variable tied to the GUI context life cycle to track IO state
    static std::filesystem::path s_last_loaded_model_path;

    try {
        // 1. Attempt to auto-populate scalars from external model metadata
        ExtractScalars(updater_config, input_config, s_last_loaded_model_path);

        // ==========================================
        // 1. FILE SOURCE CONFIGURATION
        // ==========================================
        ImGui::Spacing();
        ImGui::SeparatorText("File Source Configuration");
        ImGui::Spacing();

        if (ImGui::BeginTable("InputFileSourceTable", 2, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

            // --- Dataset Folder ---
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Dataset Folder");

                ImGui::TableSetColumnIndex(1);
                FolderInput("##DatasetFolder", input_config.file.dataset_folder, std::min(900.0f, ImGui::GetContentRegionAvail().x), "Root directory containing the map data.");
            }

            // --- Dataset Name ---
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Dataset Name");

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(std::min(400.0f, ImGui::GetContentRegionAvail().x));
                TextInput("##DatasetName", input_config.file.dataset_name, "Base name for the simulation dataset files.");
            }

            // --- Static Format ---
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Static Format");

                ImGui::TableSetColumnIndex(1);
                EnumComboBox("##StaticFormat", input_config.file.static_format, "Format of the static topography files (e.g., GeoTIFF, NetCDF).");
            }

            // --- Dynamic Format ---
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Dynamic Format");

                ImGui::TableSetColumnIndex(1);
                EnumComboBox("##DynamicFormat", input_config.file.dynamic_format, "Format of the dynamic boundary condition files.");
            }

            ImGui::EndTable();
        }

        // ==========================================
        // 2. MODEL SCALARS (METADATA EXTRACTED)
        // ==========================================
        ImGui::Spacing();
        ImGui::Spacing();
        ImGui::SeparatorText("Model Scalar Parameters");
        ImGui::Spacing();

        if (input_config.scalars.empty()) {
            ImGui::TextDisabled("No scalar parameters detected or required by the selected model.");
        }
        else {
            if (ImGui::BeginTable("InputScalarsTable", 2, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_BordersInnerH)) {
                ImGui::TableSetupColumn("Scalar Name", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                // Iterate by reference to access and modify the map values safely
                for (auto& [scalar_name, scalar_value] : input_config.scalars) {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", scalar_name.c_str());

                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(std::min(200.0f, ImGui::GetContentRegionAvail().x));

                    // Push unique ID based on scalar name to prevent ImGui state collisions dynamically
                    ImGui::PushID(scalar_name.c_str());
                    TextInput("##ScalarInput", scalar_value, "Value for this metadata-extracted scalar.");
                    ImGui::PopID();
                }

                ImGui::EndTable();
            }
        }

    }
    catch (const std::exception& e) {
        std::cerr << "[GUI Error] Exception caught while rendering Input Tab: " << e.what() << "\n";
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "An error occurred rendering the Input Tab.");
    }
    catch (...) {
        std::cerr << "[GUI Error] Unknown exception caught while rendering Input Tab.\n";
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "A critical unknown error occurred rendering the Input Tab.");
    }
}

}  // namespace floodsim::gui
