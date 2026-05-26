/**
 * @file ScenarioTab.cpp
 * @brief Implementation of the Scenario configuration tab.
 *
 * Provides the GUI layout and fields necessary to define the core
 * scenario properties such as scenario name and output directories.
 * 
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "gui/tabs/Tabs.hpp"

#include <algorithm>
#include <exception>
#include <iostream>

#include <imgui.h>

namespace floodsim::gui {

    void RenderScenarioTab(ScenarioConfig& scenario_config) {
        try {
            ImGui::Spacing();
            ImGui::SeparatorText("Basic Information");
            ImGui::Spacing();

            if (ImGui::BeginTable("BasicInfoFormTable", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

                // --- Scenario Name ---
                {
                    const char* label = "Scenario Name";
                    const char* tooltip = "Enter a unique name for this simulation run.";

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(std::min(400.0f, ImGui::GetContentRegionAvail().x));

                    // Nota: Se usa ## para ocultar el label interno de ImGui
                    TextInput("##ScenarioName", scenario_config.name, tooltip);
                }

                ImGui::EndTable();
            }

            ImGui::Spacing();
            ImGui::SeparatorText("Output Configuration");
            ImGui::Spacing();

            if (ImGui::BeginTable("OutputConfigFormTable", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

                // --- Output Directory ---
                {
                    const char* label = "Output Directory";
                    const char* tooltip = "Select the root folder where scenario results will be saved.";

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label);

                    ImGui::TableSetColumnIndex(1);
                    FolderInput("##OutputDirectory", scenario_config.output_dir, std::min(900.0f, ImGui::GetContentRegionAvail().x), tooltip);
                }

                // --- Append Timestamp ---
                {
                    const char* label = "Append Timestamp";
                    const char* tooltip = "If enabled, the simulation start time will be added to the output folder name.";

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label);

                    ImGui::TableSetColumnIndex(1);
                    Checkbox("##AppendTimestamp", scenario_config.append_start_timestamp, tooltip);
                }

                ImGui::EndTable();
            }

        }
        catch (const std::exception& e) {
            std::cerr << "[GUI Error] Exception in Scenario Tab: " << e.what() << '\n';
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "An error occurred rendering the Scenario Tab.");
        }
        catch (...) {
            std::cerr << "[GUI Error] Unknown exception in Scenario Tab.\n";
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "A critical unknown error occurred.");
        }
    }

}  // namespace floodsim::gui
