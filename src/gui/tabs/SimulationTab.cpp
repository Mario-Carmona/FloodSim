/**
 * @file SimulationTab.cpp
 * @brief Implementation of the Simulation configuration tab.
 *
 * Provides the GUI layout to configure temporal settings (such as start time,
 * time step, and duration) and spatial settings (such as the simulation view box).
 * 
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "gui/tabs/Tabs.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <exception>
#include <iostream>

#include <imgui.h>

namespace floodsim::gui {

void RenderSimulationTab(app::config::SimulationConfig& simulation_config) {
    try {
        // ==========================================
        // 1. TEMPORAL SETTINGS
        // ==========================================
        ImGui::Spacing();
        ImGui::SeparatorText("Temporal Settings");
        ImGui::Spacing();

        if (ImGui::BeginTable("TemporalConfigFormTable", 2, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

            // --- Start Timestamp ---
            {
                const char* label = "Start Timestamp";
                const char* tooltip = "Select the starting date and time for the simulation.";

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                // Uso de ## para ocultar la etiqueta nativa del widget
                TimestampPicker("##StartTimestamp", simulation_config.start_timestamp, tooltip);
            }

            // --- Time Step ---
            {
                const char* label = "Time Step";
                const char* tooltip = "Simulation integration step delta (HH:MM:SS).";

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                FractionalTimePicker("##TimeStep", simulation_config.time_step, tooltip);
            }

            // --- Duration ---
            {
                const char* label = "Duration";
                const char* tooltip = "Total duration of the simulation run (HH:MM:SS).";

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                TimePicker("##Duration", simulation_config.duration, tooltip);
            }

            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Spacing();

        // ==========================================
        // 2. SPATIAL SETTINGS (VIEW BOX)
        // ==========================================
        ImGui::SeparatorText("Spatial Settings (View Box)");
        ImGui::Spacing();

        if (ImGui::BeginTable("SpatialConfigFormTable", 2, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

            // --- Use Full Grid Checkbox ---
            {
                const char* label = "Use Full Grid";
                const char* tooltip = "If enabled, the simulation automatically spans the entire loaded topography.";

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                Checkbox("##UseFullGrid", simulation_config.view_box.use_full_grid, tooltip);
            }

            // Deshabilitar visualmente las coordenadas si el flag de "Full Grid" está activo
            bool is_full_grid = simulation_config.view_box.use_full_grid;
            if (is_full_grid) {
                ImGui::BeginDisabled();
            }

            // --- South-West Longitude ---
            {
                const char* label = "SW Longitude";
                const char* tooltip = "Longitude coordinate of the South-West point.";

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(std::min(150.0f, ImGui::GetContentRegionAvail().x));
                DoubleInput("##SWLon", simulation_config.view_box.south_west.lon, tooltip);
            }

            // --- South-West Latitude ---
            {
                const char* label = "SW Latitude";
                const char* tooltip = "Latitude coordinate of the South-West point.";

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(std::min(150.0f, ImGui::GetContentRegionAvail().x));
                DoubleInput("##SWLat", simulation_config.view_box.south_west.lat, tooltip);
            }

            // --- North-East Longitude ---
            {
                const char* label = "NE Longitude";
                const char* tooltip = "Longitude coordinate of the North-East point.";

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(std::min(150.0f, ImGui::GetContentRegionAvail().x));
                DoubleInput("##NELon", simulation_config.view_box.north_east.lon, tooltip);
            }

            // --- North-East Latitude ---
            {
                const char* label = "NE Latitude";
                const char* tooltip = "Latitude coordinate of the North-East point.";

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(std::min(150.0f, ImGui::GetContentRegionAvail().x));
                DoubleInput("##NELat", simulation_config.view_box.north_east.lat, tooltip);
            }

            if (is_full_grid) {
                ImGui::EndDisabled();
            }

            ImGui::EndTable();
        }

    }
    catch (const std::exception& e) {
        std::cerr << "[GUI Error] Exception in Simulation Tab: " << e.what() << '\n';
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "An error occurred rendering the Simulation Tab.");
    }
    catch (...) {
        std::cerr << "[GUI Error] Unknown exception in Simulation Tab.\n";
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "A critical unknown error occurred.");
    }
}

}  // namespace floodsim::gui
