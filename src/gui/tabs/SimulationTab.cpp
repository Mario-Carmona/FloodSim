/**
 * \file SimulationTab.cpp
 * \brief Implementation of the Simulation configuration tab.
 *
 * Provides the GUI layout to configure temporal settings (such as start time,
 * time step, and duration) and spatial settings (such as the simulation view box).
 */

#include "gui/tabs/Tabs.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <exception>
#include <iostream>

#include <imgui.h>

namespace floodsim::gui {

    void RenderSimulationTab(danasim::SimulationConfig& simulation_config) {
        try {
            ImGui::Spacing();
            ImGui::SeparatorText("Temporal Settings");
            ImGui::Spacing();

            if (ImGui::BeginTable("TemporalConfigFormTable", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

                {
                    const char* label = "Start Timestamp";
                    const char* tooltip = "Select the starting date and time for the simulation.";

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(std::min(200.0f, ImGui::GetContentRegionAvail().x));
                    TimestampPicker(label, simulation_config.startTimestamp, tooltip);
                }

                {
                    const char* label = "Time Step (s)";
                    const char* tooltip = "Simulation step delta in seconds. (e.g., 1 for 00:00:01)";

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(std::min(200.0f, ImGui::GetContentRegionAvail().x));
                    TimePicker(label, simulation_config.timeStep, tooltip);

                    // Safety check to ensure time step is at least 1 second
                    if (simulation_config.timeStep.count() < 1) {
                        simulation_config.timeStep = std::chrono::seconds(1);
                    }
                }

                {
                    const char* label = "Duration (s)";
                    const char* tooltip = "Total simulation duration in seconds. (e.g., 14400 for 04:00:00)";

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(std::min(200.0f, ImGui::GetContentRegionAvail().x));
                    TimePicker(label, simulation_config.duration, tooltip);

                    // Safety check to ensure duration is at least 1 second
                    if (simulation_config.duration.count() < 1) {
                        simulation_config.duration = std::chrono::seconds(1);
                    }
                }

                ImGui::EndTable();
            }

            ImGui::Spacing();
            ImGui::SeparatorText("Spatial Settings (ViewBox)");
            ImGui::Spacing();

            if (ImGui::BeginTable("SpatialConfigFormTable", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

                // --- Use Full Grid ---
                {
                    const char* label = "Use Full Grid";
                    const char* tooltip = "If enabled, the simulation runs over the entire loaded grid area.";

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label);

                    ImGui::TableSetColumnIndex(1);
                    Checkbox(label, simulation_config.viewBox.useFullGrid, tooltip);
                }

                // --- Bounding Box Coordinates ---
                // Draw these inputs only if "Use Full Grid" is disabled
                if (simulation_config.viewBox.useFullGrid) {
                    simulation_config.viewBox.southWest.lon = 0.0;
                    simulation_config.viewBox.southWest.lat = 0.0;

                    simulation_config.viewBox.northEast.lon = 0.0;
                    simulation_config.viewBox.northEast.lat = 0.0;
                }
                else {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);

                    ImGui::BulletText("South West:");
                    ImGui::Spacing();

                    // South-West Lon
                    {
                        const char* label = "SW Longitude";
                        const char* tooltip = "Longitude coordinate of the South-West point.";

                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("%s", label);

                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(std::min(100.0f, ImGui::GetContentRegionAvail().x));
                        DoubleInput(label, simulation_config.viewBox.southWest.lon, tooltip);
                    }

                    // South-West Lat
                    {
                        const char* label = "SW Latitude";
                        const char* tooltip = "Latitude coordinate of the South-West point.";

                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("%s", label);

                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(std::min(100.0f, ImGui::GetContentRegionAvail().x));
                        DoubleInput(label, simulation_config.viewBox.southWest.lat, tooltip);
                    }

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);

                    ImGui::Spacing();
                    ImGui::BulletText("North East:");
                    ImGui::Spacing();

                    // North-East Lon
                    {
                        const char* label = "NE Longitude";
                        const char* tooltip = "Longitude coordinate of the North-East point.";

                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("%s", label);

                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(std::min(100.0f, ImGui::GetContentRegionAvail().x));
                        DoubleInput(label, simulation_config.viewBox.northEast.lon, tooltip);
                    }

                    // North-East Lat
                    {
                        const char* label = "NE Latitude";
                        const char* tooltip = "Latitude coordinate of the North-East point.";

                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("%s", label);

                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(std::min(100.0f, ImGui::GetContentRegionAvail().x));
                        DoubleInput(label, simulation_config.viewBox.northEast.lat, tooltip);
                    }
                }

                ImGui::EndTable();
            }

        }
        catch (const std::exception& e) {
            std::cerr << "[Error] Exception caught while rendering Simulation Tab: " << e.what() << "\n";
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "An error occurred rendering the Simulation Tab.");
        }
        catch (...) {
            std::cerr << "[Error] Unknown exception caught while rendering Simulation Tab.\n";
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "A critical unknown error occurred rendering the Simulation Tab.");
        }
    }

}  // namespace floodsim::gui
