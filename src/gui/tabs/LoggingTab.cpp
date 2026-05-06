
#include "gui/tabs/Tabs.hpp"

#include "logging/LoggerLevel.hpp"

namespace FloodSim::Gui {

	void renderLoggingTab(danasim::LoggingConfig& loggingConfig) {
        ImGui::Spacing();
        ImGui::SeparatorText("System Logging");
        ImGui::Spacing();

        if (ImGui::BeginTable("LoggingFormTable", 2, ImGuiTableFlags_SizingStretchProp)) {

            // Configuramos el ancho de la columna izquierda
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

            {
                const char* label = "Log Level";
                const char* tooltip = "Set the minimum severity level for log messages to be recorded.";

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(std::min(120.0f, ImGui::GetContentRegionAvail().x));
                EnumComboBox<LoggerLevel>(label, loggingConfig.level, tooltip);
            }

            {
                const char* label = "Asynchronous Logging";
                const char* tooltip = "Enable non-blocking asynchronous logging. Recommended for performance.";

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                Checkbox(label, loggingConfig.async, tooltip);
            }

            {
                const char* label = "Silent Mode";
                const char* tooltip = "Disable console output. Logs will not be printed to the terminal.";

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                Checkbox(label, loggingConfig.silent, tooltip);
            }

            {
                const char* label = "Save Log File";
                const char* tooltip = "Write log output to a file within the Scenario Output Directory.";

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                Checkbox(label, loggingConfig.saveLogFile, tooltip);
            }

            ImGui::EndTable();
        }
	}

}
