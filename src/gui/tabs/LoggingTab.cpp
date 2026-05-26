/**
 * @file LoggingTab.cpp
 * @brief Implementation of the Logging configuration tab.
 *
 * Provides the GUI layout to configure the system logging settings,
 * including log levels, asynchronous logging, and file output.
 * 
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "gui/tabs/Tabs.hpp"

#include <algorithm>
#include <exception>
#include <iostream>

#include <imgui.h>

#include "logging/LoggerLevel.hpp"

namespace floodsim::gui {

void RenderLoggingTab(app::config::LoggingConfig& logging_config) {
    try {
        ImGui::Spacing();
        ImGui::SeparatorText("System Logging");
        ImGui::Spacing();

        if (ImGui::BeginTable("LoggingFormTable", 2, ImGuiTableFlags_SizingStretchProp)) {

            // Set up column widths
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

            // --- Log Level ---
            {
                const char* label = "Log Level";
                const char* tooltip = "Set the minimum severity level for log messages to be recorded.";

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                // Uso del helper inteligente que limpiará el prefijo 'k' del enum
                EnumComboBox("##LogLevel", logging_config.level, tooltip);
            }

            // --- Asynchronous Logging ---
            {
                const char* label = "Asynchronous";
                const char* tooltip = "Enable asynchronous logging to improve simulation performance.";

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                Checkbox("##Asynchronous", logging_config.async, tooltip);
            }

            // --- Silent Mode ---
            {
                const char* label = "Silent Mode";
                const char* tooltip = "Disable all log output (both console and file).";

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                Checkbox("##SilentMode", logging_config.silent, tooltip);
            }

            // --- Save Log File ---
            {
                const char* label = "Save Log File";
                const char* tooltip = "Write log output to a file within the Scenario Output Directory.";

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                Checkbox("##SaveLogFile", logging_config.save_log_file, tooltip);
            }

            ImGui::EndTable();
        }

    }
    catch (const std::exception& e) {
        std::cerr << "[GUI Error] Exception in Logging Tab: " << e.what() << '\n';
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "An error occurred rendering the Logging Tab.");
    }
    catch (...) {
        std::cerr << "[GUI Error] Unknown exception in Logging Tab.\n";
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "A critical unknown error occurred rendering the Logging Tab.");
    }
}

}  // namespace floodsim::gui
