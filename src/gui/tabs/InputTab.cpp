/**
 * \file InputTab.cpp
 * \brief Implementation of the Input Data configuration tab.
 *
 * Provides the GUI layout to configure input sources for the simulation,
 * including static topography, dynamic boundary conditions, and auto-extracted
 * scalars from the selected AI model's metadata.
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

#include "app/adapters/input/readers/file/dynamic/FileDynamicFormat.hpp"
#include "app/adapters/input/readers/file/static/FileStaticFormat.hpp"

namespace {

    /**
     * \brief Extracts scalar configuration parameters from a model's JSON metadata.
     * * Inspects the currently selected AI model path (e.g., ONNX model folder),
     * looks for a 'metadata.json', and auto-populates the required scalar inputs.
     * * \param updater_config Read-only reference to the state updater configuration.
     * \param input_config The input configuration to modify with discovered scalars.
     * \param last_loaded_model_path Tracks the last processed model to avoid redundant disk I/O.
     */
    void ExtractScalars(const danasim::UpdaterConfig& updater_config,
        danasim::InputConfig& input_config,
        std::string& last_loaded_model_path) {
        try {
            std::string current_model_path;

            // Retrieve the current model path safely from the variant
            std::visit([&current_model_path](const auto& cfg) {
                    using T = std::decay_t<decltype(cfg)>;
                    if constexpr (std::is_same_v<T, danasim::OnnxUpdaterConfig>) {
                        current_model_path = cfg.modelPath.string();
                    }
                }, 
                updater_config
            );

            // Only process if the path has changed and is not empty
            if (!current_model_path.empty() && current_model_path != last_loaded_model_path) {
                last_loaded_model_path = current_model_path;
                std::unordered_set<std::string> expected_scalars;

                std::visit([&expected_scalars](const auto& cfg) {
                        using T = std::decay_t<decltype(cfg)>;
                        if constexpr (std::is_same_v<T, danasim::OnnxUpdaterConfig>) {
                            std::filesystem::path config_path = cfg.modelPath / "metadata.json";

                            if (std::filesystem::exists(config_path)) {
                                std::ifstream file(config_path);
                                if (file.is_open()) {
                                    nlohmann::json j;
                                    file >> j; // May throw if JSON is malformed

                                    for (const char* method : { "preprocess", "step" }) {
                                        for (const auto& input : j[method]["inputs"]) {
                                            // La regla de oro: Si el shape está vacío, es un escalar
                                            if (input["shape"].empty()) {
                                                std::string scalarName = input["name"];
                                                expected_scalars.insert(scalarName);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }, 
                    updater_config
                );

                for (const char* implicit_scalar : { "delta_x", "delta_t" }) {
                    expected_scalars.erase(implicit_scalar);
                }

                // 1. Remove scalars that are no longer expected by the new model
                for (auto it = input_config.scalars.begin(); it != input_config.scalars.end(); ) {
                    if (expected_scalars.find(it->first) == expected_scalars.end()) {
                        it = input_config.scalars.erase(it);
                    }
                    else {
                        ++it;
                    }
                }

                // 2. Add new expected scalars, initializing them with empty strings
                for (const auto& scalar : expected_scalars) {
                    if (input_config.scalars.find(scalar) == input_config.scalars.end()) {
                        input_config.scalars[scalar] = "";
                    }
                }
            }
        }
        catch (const nlohmann::json::exception& e) {
            std::cerr << "[Warning] JSON parsing error in ExtractScalars: " << e.what() << "\n";
        }
        catch (const std::exception& e) {
            std::cerr << "[Warning] Unexpected error in ExtractScalars: " << e.what() << "\n";
        }
    }

} // anonymous namespace

namespace floodsim::gui {

    void RenderInputTab(danasim::InputConfig& input_config, const danasim::UpdaterConfig& updater_config) {
        try {
            ImGui::Spacing();
            ImGui::SeparatorText("Static Topography");
            ImGui::Spacing();

            if (ImGui::BeginTable("InputStaticTable", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

                {
                    const char* label = "Static Format";
                    const char* tooltip = "Data format for static spatial maps (e.g., topography DEMs).";

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(std::min(120.0f, ImGui::GetContentRegionAvail().x));
                    EnumComboBox<FileStaticFormat>(label, input_config.file.staticFormat, tooltip);
                }

                {
                    const char* label = "Dynamic Format";
                    const char* tooltip = "Data format for dynamic time-series (e.g., rainfall over time).";

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(std::min(120.0f, ImGui::GetContentRegionAvail().x));
                    EnumComboBox<FileDynamicFormat>(label, input_config.file.dynamicFormat, tooltip);
                }

                {
                    const char* label = "Dataset Name";
                    const char* tooltip = "Name of the input dataset.";

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(std::min(400.0f, ImGui::GetContentRegionAvail().x));
                    TextInput(label, input_config.file.datasetName, tooltip);
                }

                ImGui::EndTable();
            }

            ImGui::Spacing();
            ImGui::SeparatorText("Scalars (Auto-detected)");
            ImGui::Spacing();

            static std::string last_loaded_model_path = "";

            // Attempt to extract new scalars based on the current model path
            ExtractScalars(updater_config, input_config, last_loaded_model_path);

            if (input_config.scalars.empty()) {
                ImGui::TextDisabled("No scalars required by the current model.");
            }
            else {
                if (ImGui::BeginTable("InputScalarsTable", 2, ImGuiTableFlags_SizingStretchProp)) {
                    ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                    ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

                    // Use structured binding to access and modify the map values safely
                    for (auto& [scalar_name, scalar_value] : input_config.scalars) {
                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("%s", scalar_name.c_str());

                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(std::min(200.0f, ImGui::GetContentRegionAvail().x));

                        // Push unique ID based on scalar name to prevent ImGui state collisions
                        ImGui::PushID(scalar_name.c_str());
                        TextInput("##ScalarInput", scalar_value, "Extracted from model metadata.");
                        ImGui::PopID();
                    }

                    ImGui::EndTable();
                }
            }

        }
        catch (const std::exception& e) {
            std::cerr << "[Error] Exception caught while rendering Input Tab: " << e.what() << "\n";
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "An error occurred rendering the Input Tab.");
        }
        catch (...) {
            std::cerr << "[Error] Unknown exception caught while rendering Input Tab.\n";
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "A critical unknown error occurred rendering the Input Tab.");
        }
    }

}  // namespace floodsim::gui
