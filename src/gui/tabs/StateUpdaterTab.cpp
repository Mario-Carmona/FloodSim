/**
 * @file StateUpdaterTab.cpp
 * @brief Implementation of the State Updater configuration tab.
 *
 * Provides the GUI layout to configure the physical or AI-based models
 * that update the simulation state, as well as the dynamic flood risk
 * visualization levels.
 * 
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "gui/tabs/Tabs.hpp"

#include <algorithm>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <variant>
#include <vector>

#include <imgui.h>

namespace {

    /**
     * @brief C++20 Visitor Helper for std::visit.
     * @details Allows passing multiple lambda expressions to handle different
     * types inside a std::variant seamlessly.
     */
    template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

}  // namespace (anonymous)

namespace floodsim::gui {

void RenderStateUpdaterTab(app::config::StateUpdaterConfig& state_updater_config) {
    try {
        // ==========================================
        // 1. GENERAL UPDATER SETTINGS
        // ==========================================
        ImGui::Spacing();
        ImGui::SeparatorText("General Settings");
        ImGui::Spacing();

        if (ImGui::BeginTable("StateUpdaterGeneralTable", 2, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

            // --- Enable Rainfall ---
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Enable Rainfall");

                ImGui::TableSetColumnIndex(1);
                Checkbox("##EnableRainfall", state_updater_config.enable_rainfall, "Toggle rainfall integration in the simulation step.");
            }

            // --- Dry Tolerance ---
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Dry Tolerance (m)");

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(150.0f);
                FloatInput("##DryTolerance", state_updater_config.dry_tolerance, "Water depth threshold below which a cell is considered dry.");
            }

            ImGui::EndTable();
        }

        // ==========================================
        // 2. UPDATER ENGINE CONFIGURATION
        // ==========================================
        ImGui::Spacing();
        ImGui::SeparatorText("Simulation Engine");
        ImGui::Spacing();

        if (ImGui::BeginTable("UpdaterEngineTable", 2, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

            // --- Engine Type Selection ---
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Engine Type");

            ImGui::TableSetColumnIndex(1);

            // Detect current type from the variant
            app::config::UpdaterConfigType current_type = std::visit(Overloaded{
                [](const app::config::OnnxUpdaterConfig&) { return app::config::UpdaterConfigType::kOnnx; }
                }, state_updater_config.updater);

            app::config::UpdaterConfigType selected_type = current_type;
            EnumComboBox("##UpdaterType", selected_type, "Select the computational backend for state updates.");

            // If the user changed the type, re-initialize the variant
            if (selected_type != current_type) {
                if (selected_type == app::config::UpdaterConfigType::kOnnx) {
                    state_updater_config.updater = app::config::OnnxUpdaterConfig{};
                }
            }

            // --- Engine Specific Parameters ---
            std::visit(Overloaded{
                [](app::config::OnnxUpdaterConfig& onnx_cfg) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Model Path");

                    ImGui::TableSetColumnIndex(1);
                    FolderInput("##OnnxModelPath", onnx_cfg.model_path, ImGui::GetContentRegionAvail().x, "Path to the .onnx file or model directory.");

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Tensor Dimension");

                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(150.0f);
                    // Using native InputScalar for int64_t to ensure 64-bit safety
                    ImGui::InputScalar("##TensorDim", ImGuiDataType_S64, &onnx_cfg.tensor_dim);
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Internal tensor dimension (spatial resolution square size).");
                    }
                }
                }, state_updater_config.updater);

            ImGui::EndTable();
        }

        // ==========================================
        // 3. FLOOD RISK LEVELS (DYNAMIC LIST)
        // ==========================================
        ImGui::Spacing();
        ImGui::SeparatorText("Flood Risk Levels (Visualization)");
        ImGui::Spacing();

        int item_to_delete = -1;

        for (size_t i = 0; i < state_updater_config.flood_risk_levels.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));

            auto& level = state_updater_config.flood_risk_levels[i];
            bool is_default = (i == 0);

            if (is_default) {
                ImGui::Text("Default Level (Base Terrain)");
            }
            else {
                ImGui::Text("Risk Level #%d", static_cast<int>(i));
                ImGui::SameLine(ImGui::GetWindowWidth() - 90.0f);

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                if (ImGui::Button("Delete", ImVec2(70, 0))) {
                    item_to_delete = static_cast<int>(i);
                }
                ImGui::PopStyleColor(2);
            }

            if (ImGui::BeginTable("RiskLevelTable", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

                // Name Input
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Level Name");

                ImGui::TableSetColumnIndex(1);
                TextInput("##RiskName", level.name, "Friendly name for this risk level.");

                // Threshold Input (Hidden for the default level, since it's always 0.0)
                if (!is_default) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("Threshold Start (m)");

                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(150.0f);
                    FloatInput("##RiskThreshold", level.threshold_start, "Minimum water depth required to trigger this level.");
                }

                // Color Input
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("Level Color");

                ImGui::TableSetColumnIndex(1);
                ColorInput("##RiskColor", level.color_hex, "Hex color code for rendering.");

                ImGui::EndTable();
            }

            ImGui::Separator();
            ImGui::Spacing();
            ImGui::PopID();
        }

        // Execute deletion safely outside the render loop
        // Note: item_to_delete will never be 0 because we don't render the Delete button for index 0
        if (item_to_delete > 0) {
            state_updater_config.flood_risk_levels.erase(state_updater_config.flood_risk_levels.begin() + item_to_delete);
        }

        if (ImGui::Button("+ Add Risk Level", ImVec2(-FLT_MIN, 40))) {
            app::config::FloodRiskLevel new_level;
            new_level.name = "New Level";
            new_level.threshold_start = 1.0f; // Safe default threshold
            new_level.color_hex = "#FFFFFF";
            state_updater_config.flood_risk_levels.push_back(new_level);
        }

        // Automatic Sorting
        // We sort the list based on the threshold, starting from element 1 to protect the Default level.
        // Crucial: Only sort if NO active item is currently being edited to avoid the UI jumping under the cursor.
        if (!ImGui::IsAnyItemActive() && state_updater_config.flood_risk_levels.size() > 1) {
            std::sort(
                state_updater_config.flood_risk_levels.begin() + 1,
                state_updater_config.flood_risk_levels.end(),
                [](const app::config::FloodRiskLevel& a, const app::config::FloodRiskLevel& b) {
                    return a.threshold_start < b.threshold_start;
                }
            );
        }

    }
    catch (const std::exception& e) {
        std::cerr << "[GUI Error] Exception caught in State Updater Tab: " << e.what() << "\n";
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "An error occurred rendering the State Updater Tab.");
    }
    catch (...) {
        std::cerr << "[GUI Error] Unknown exception caught in State Updater Tab.\n";
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "A critical unknown error occurred rendering the State Updater Tab.");
    }
}

}  // namespace floodsim::gui
