/**
 * \file LoggingTab.cpp
 * \brief Implementation of the Logging configuration tab.
 *
 * Provides the GUI layout to configure the system logging settings,
 * including log levels, asynchronous logging, and file output.
 */

#include "gui/tabs/Tabs.hpp"

#include <algorithm>
#include <exception>
#include <iostream>

#include <imgui.h>

#include "logging/LoggerLevel.hpp"

namespace floodsim::gui {

    void RenderLoggingTab(danasim::LoggingConfig& logging_config) {
        try {
            ImGui::Spacing();
            ImGui::SeparatorText("System Logging");
            ImGui::Spacing();

            if (ImGui::BeginTable("LoggingFormTable", 2, ImGuiTableFlags_SizingStretchProp)) {

                // Set up column widths
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

                    // Assuming LoggerLevel is an enum class mapped with magic_enum in EnumComboBox
                    EnumComboBox<LoggerLevel>(label, logging_config.level, tooltip);
                }

                {
                    const char* label = "Asynchronous Logging";
                    const char* tooltip = "Enable non-blocking asynchronous logging. Recommended for performance.";

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label);

                    ImGui::TableSetColumnIndex(1);
                    Checkbox(label, logging_config.async, tooltip);
                }

                {
                    const char* label = "Silent Mode";
                    const char* tooltip = "Disable console output. Logs will not be printed to the terminal.";

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label);

                    ImGui::TableSetColumnIndex(1);
                    Checkbox(label, logging_config.silent, tooltip);
                }

                {
                    const char* label = "Save Log File";
                    const char* tooltip = "Write log output to a file within the Scenario Output Directory.";

                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label);

                    ImGui::TableSetColumnIndex(1);
                    Checkbox(label, logging_config.saveLogFile, tooltip);
                }

                ImGui::EndTable();
            }

        }
        catch (const std::exception& e) {
            std::cerr << "[Error] Exception caught while rendering Logging Tab: " << e.what() << "\n";
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "An error occurred rendering the Logging Tab.");
        }
        catch (...) {
            std::cerr << "[Error] Unknown exception caught while rendering Logging Tab.\n";
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "A critical unknown error occurred rendering the Logging Tab.");
        }
    }

}  // namespace floodsim::gui
