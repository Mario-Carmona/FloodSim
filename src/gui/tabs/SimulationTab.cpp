
#include "gui/tabs/Tabs.hpp"

#include <chrono>
#include <ctime>

namespace FloodSim::Gui {

	void renderSimulationTab(danasim::SimulationConfig& simulationConfig) {
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
                TimestampPicker(label, simulationConfig.startTimestamp, tooltip);
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
                TimePicker(label, simulationConfig.timeStep, tooltip);
                if (simulationConfig.timeStep.count() < 1) simulationConfig.timeStep = std::chrono::seconds(1);
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
                TimePicker(label, simulationConfig.duration, tooltip);
                if (simulationConfig.duration.count() < 1) simulationConfig.duration = std::chrono::seconds(1);
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
                // Usamos tu Helper (modificará directamente simulationConfig.viewBox.useFullGrid)
                Checkbox(label, simulationConfig.viewBox.useFullGrid, tooltip);
            }

            // --- Bounding Box Coordinates ---
            // Solo dibujamos estos inputs si el usuario DESMARCA "Use Full Grid"
            if (simulationConfig.viewBox.useFullGrid) {
                simulationConfig.viewBox.southWest.lon = 0.0;
                simulationConfig.viewBox.southWest.lat = 0.0;

                simulationConfig.viewBox.northEast.lon = 0.0;
                simulationConfig.viewBox.northEast.lat = 0.0;
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
                    DoubleInput(label, simulationConfig.viewBox.southWest.lon, tooltip);
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
                    DoubleInput(label, simulationConfig.viewBox.southWest.lat, tooltip);
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
                    DoubleInput(label, simulationConfig.viewBox.northEast.lon, tooltip);
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
                    DoubleInput(label, simulationConfig.viewBox.northEast.lat, tooltip);
                }
            }

            ImGui::EndTable();
        }
	}

}
