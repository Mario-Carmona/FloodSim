/**
 * \file StateUpdaterTab.cpp
 * \brief Implementation of the State Updater configuration tab.
 *
 * Provides the GUI layout to configure the physical or AI-based models
 * that update the simulation state, as well as the dynamic flood risk
 * visualization levels.
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
     * \brief C++20 Visitor Helper for std::visit.
     * \details Allows passing multiple lambda expressions to handle different
     * types inside a std::variant seamlessly.
     */
    template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

}  // namespace

namespace floodsim::gui {

    void RenderStateUpdaterTab(danasim::StateUpdaterConfig& state_updater_config) {
        try {
            ImGui::Spacing();
            ImGui::SeparatorText("General");
            ImGui::Spacing();

            if (ImGui::BeginTable("StateUpdaterGeneralTable", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

                {
                    const char* label = "Enable Rainfall";
                    const char* tooltip = "Enable or disable rainfall contribution to the simulation state.";

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label);

                    ImGui::TableSetColumnIndex(1);
                    Checkbox(label, state_updater_config.enableRainfall, tooltip);
                }

                {
                    const char* label = "Dry Tolerance";
                    const char* tooltip = "Minimum depth threshold to treat a grid cell as dry.";

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(std::min(100.0f, ImGui::GetContentRegionAvail().x));
                    FloatInput(label, state_updater_config.dryTolerance, tooltip);
                }

                ImGui::EndTable();
            }

            ImGui::Spacing();
            ImGui::SeparatorText("Model Updater");
            ImGui::Spacing();

            if (ImGui::BeginTable("StateUpdaterModelTable", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

                {
                    const char* label = "Model Type";
                    const char* tooltip = "Select the type of mathematical/AI model for state updating.";

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(std::min(120.0f, ImGui::GetContentRegionAvail().x));

                    // 1. INFER ENUM FROM ACTIVE VARIANT
                    // Read which struct type is currently held in the variant
                    danasim::UpdaterConfigType current_type;

                    std::visit(Overloaded{
                            [&current_type](danasim::OnnxUpdaterConfig&) {
                                current_type = danasim::UpdaterConfigType::ONNX;
                            },
                            // Future types can be handled here
                            [](auto&&) {
                                throw std::runtime_error("StateUpdaterTab: Unimplemented updater type.");
                            }
                        },
                        state_updater_config.updater
                    );

                    danasim::UpdaterConfigType prev_type = current_type;
                    EnumComboBox<danasim::UpdaterConfigType>(label, current_type, tooltip);

                    if (current_type != prev_type) {
                        switch (current_type) {
                        case danasim::UpdaterConfigType::ONNX:
                            state_updater_config.updater = danasim::OnnxUpdaterConfig{};
                            break;
                            // Add future cases here
                        }
                    }
                }

                // 2. RENDER SPECIFIC UI BASED ON VARIANT TYPE
                std::visit(Overloaded{
                        // --- ONNX Strategy ---
                        [](danasim::OnnxUpdaterConfig& onnx_config) {
                            {
                                const char* label = "Model Path";
                                const char* tooltip = "Directory containing the exported ONNX model.";

                                ImGui::TableNextRow();

                                ImGui::TableSetColumnIndex(0);
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text("%s", label);

                                ImGui::TableSetColumnIndex(1);
                                FolderInput(label, onnx_config.modelPath, std::min(900.0f, ImGui::GetContentRegionAvail().x), tooltip);
                            }

                            {
                                const char* label = "Tensor Dimension";
                                const char* tooltip = "Size of the N-dimensional tensor.";

                                ImGui::TableNextRow();

                                ImGui::TableSetColumnIndex(0);
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text("%s", label);

                                ImGui::TableSetColumnIndex(1);
                                ImGui::SetNextItemWidth(std::min(100.0f, ImGui::GetContentRegionAvail().x));

                                // Define exact allowed values
                                const std::vector<int64_t> allowed_tensor_dims = { 16, 32, 64, 128, 256, 512, 1024 };

                                // Safety check: force to first valid option if current is invalid
                                if (std::find(allowed_tensor_dims.begin(), allowed_tensor_dims.end(), onnx_config.tensorDim) == allowed_tensor_dims.end()) {
                                    onnx_config.tensorDim = allowed_tensor_dims.front();
                                }

                                ComboBox<int64_t>(label, onnx_config.tensorDim, allowed_tensor_dims, tooltip);
                            }
                        },

                        // --- Catch-all (Fallback for missing GUI implementations) ---
                        [](auto&&) {
                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::TextDisabled("Configuration");

                            ImGui::TableSetColumnIndex(1);
                            ImGui::TextDisabled("No specific settings for this updater type.");
                        }
                    }, 
                    state_updater_config.updater
                );

                ImGui::EndTable();
            }

            ImGui::Spacing();
            ImGui::SeparatorText("Flood Risk Levels");
            ImGui::Spacing();

            // --- 1. DEFAULT LEVEL (Always Index 0) ---
            ImGui::BulletText("Default Level (Threshold: 0.0):");
            ImGui::Spacing();

            if (ImGui::BeginTable("DefaultLevelTable", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

                // Safety verification: Ensure at least the default level exists
                if (state_updater_config.floodRiskLevels.empty()) {
                    state_updater_config.floodRiskLevels.resize(1);
                }

                auto& default_lvl = state_updater_config.floodRiskLevels[0];

                {
                    const char* label = "Name";
                    const char* tooltip = "Name of the default dry state.";

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(std::min(300.0f, ImGui::GetContentRegionAvail().x));

                    TextInput(label, default_lvl.name, tooltip);
                }

                {
                    const char* label = "Color (Hex)";
                    const char* tooltip = "Base color for dry terrain.";

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(std::min(120.0f, ImGui::GetContentRegionAvail().x));
                    ColorInput(label, default_lvl.colorHex, tooltip);
                }

                ImGui::EndTable();
            }

            ImGui::Spacing();
            ImGui::BulletText("Dynamic Threshold Levels:");
            ImGui::Spacing();

            if (ImGui::Button(" + Add New Level ")) {
                state_updater_config.floodRiskLevels.push_back({ "New Level", 0.01f, "FFFFFFFF" });
            }

            ImGui::Spacing();

            // --- 2. DYNAMIC LEVELS TABLE ---
            if (ImGui::BeginTable("DynamicLevelsTable", 4,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoKeepColumnsVisible,
                ImVec2(710.0f, 0.0f))) {

                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 300.0f);
                ImGui::TableSetupColumn("Threshold (Start)", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthFixed, 120.0f);
                ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 70.0f);
                ImGui::TableHeadersRow();

                int item_to_delete = -1;

                for (size_t i = 1; i < state_updater_config.floodRiskLevels.size(); ++i) {
                    auto& level = state_updater_config.floodRiskLevels[i];

                    ImGui::PushID(static_cast<int>(i));
                    ImGui::TableNextRow();

                    // Column 1: Name
                    ImGui::TableSetColumnIndex(0);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    {
                        const char* label = "Name";
                        const char* tooltip = "Descriptive name for this flood level.";
                        TextInput(label, level.name, tooltip);
                    }

                    // Column 2: Threshold
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    {
                        const char* label = "Threshold";
                        const char* tooltip = "Minimum water depth (in meters) to trigger this risk level. Values must be > 0.";
                        FloatInput(label, level.thresholdStart, tooltip);

                        if (level.thresholdStart <= 0.0f) {
                            level.thresholdStart = 0.0001f;
                        }
                    }

                    // Column 3: Color Inline
                    ImGui::TableSetColumnIndex(2);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    {
                        const char* label = "Color";
                        const char* tooltip = "Visual color representing this depth level on the simulation map.";
                        ColorInput(label, level.colorHex, tooltip);
                    }

                    // Column 4: Delete Button
                    ImGui::TableSetColumnIndex(3);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                    if (ImGui::Button("Remove", ImVec2(-FLT_MIN, 0))) {
                        item_to_delete = static_cast<int>(i);
                    }
                    ImGui::PopStyleColor(2);

                    ImGui::PopID();
                }

                ImGui::EndTable();

                if (item_to_delete >= 1) {
                    state_updater_config.floodRiskLevels.erase(
                        state_updater_config.floodRiskLevels.begin() + item_to_delete
                    );
                }
            }

            // --- 3. AUTOMATIC SORTING ---
            // We sort the list based on the threshold, BUT starting from element 1.
            // This ensures element 0 (Default) never moves or gets lost.
            // We only sort if NO active item is currently being edited by the user to avoid jumping cursors.
            if (!ImGui::IsAnyItemActive() && state_updater_config.floodRiskLevels.size() > 1) {
                std::sort(state_updater_config.floodRiskLevels.begin() + 1,
                    state_updater_config.floodRiskLevels.end(),
                    [](const auto& a, const auto& b) {
                        return a.thresholdStart < b.thresholdStart;
                    }
                );
            }

        }
        catch (const std::exception& e) {
            std::cerr << "[Error] Exception caught while rendering State Updater Tab: " << e.what() << "\n";
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "An error occurred rendering the State Updater Tab: %s", e.what());
        }
        catch (...) {
            std::cerr << "[Error] Unknown exception caught while rendering State Updater Tab.\n";
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "A critical unknown error occurred rendering the State Updater Tab.");
        }
    }

}  // namespace floodsim::gui
