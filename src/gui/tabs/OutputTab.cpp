/**
 * @file OutputTab.cpp
 * @brief Implementation of the Output configuration tab.
 *
 * Provides the GUI layout to configure simulation outputs, including
 * snapshot frequencies and multiple dynamic output destinations.
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
     * @details Essential for unboxing configuration structs from std::variant cleanly.
     */
    template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

} // namespace

namespace floodsim::gui {

void RenderOutputTab(OutputConfig& output_config) {
    try {
        // ==========================================
        // 1. SNAPSHOT CONFIGURATION
        // ==========================================
        ImGui::Spacing();
        ImGui::SeparatorText("Snapshot Configuration");
        ImGui::Spacing();

        if (ImGui::BeginTable("SnapshotConfigTable", 2, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

            {
                const char* label = "Every N Steps";
                const char* tooltip = "Save a snapshot every N simulation steps.";

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(std::min(150.0f, ImGui::GetContentRegionAvail().x));

                Int64Input(label, output_config.snapshot.every_n_steps, tooltip);

                // Safety clamp to ensure a valid frequency
                if (output_config.snapshot.every_n_steps < 1) {
                    output_config.snapshot.every_n_steps = 1;
                }
            }

            ImGui::EndTable();
        }

        // ==========================================
        // 2. DYNAMIC OUTPUTS DESTINATIONS
        // ==========================================
        ImGui::Spacing();
        ImGui::SeparatorText("Configured Outputs");
        ImGui::Spacing();

        int item_to_delete = -1;

        for (size_t i = 0; i < output_config.outputs.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));

            ImGui::Text("Output #%zu", i + 1);
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 70.0f);

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
            if (ImGui::Button("Delete", ImVec2(70.0f, 0))) {
                item_to_delete = static_cast<int>(i);
            }
            ImGui::PopStyleColor(2);

            if (ImGui::BeginTable("OutputSettingsTable", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

                {
                    const char* label = "Output Type";
                    const char* tooltip = "Select the type of output to configure.";

                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", label);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(std::min(150.0f, ImGui::GetContentRegionAvail().x));

                    // Determine the current type inspecting the std::variant
                    OutputConfig::OutputConfigEntryType current_type;
                    std::visit(Overloaded{
                            [&current_type](OutputConfig::CheckpointOutputConfigEntry&) { current_type = OutputConfig::OutputConfigEntryType::kCheckpoint; },
                            [&current_type](OutputConfig::MqttOutputConfigEntry&) { current_type = OutputConfig::OutputConfigEntryType::kMqtt; },
                            [&current_type](OutputConfig::ImageOutputConfigEntry&) { current_type = OutputConfig::OutputConfigEntryType::kImage; },
                            [](auto&&) { throw std::runtime_error("OutputTab: Unimplemented output type variant."); }
                        },
                        output_config.outputs[i]
                    );

                    OutputConfig::OutputConfigEntryType prev_type = current_type;
                    EnumComboBox<OutputConfig::OutputConfigEntryType>(label, current_type, tooltip);

                    // Re-assign a default-constructed struct to the variant if the type changed
                    if (current_type != prev_type) {
                        switch (current_type) {
                        case OutputConfig::OutputConfigEntryType::kCheckpoint:
                            output_config.outputs[i] = OutputConfig::CheckpointOutputConfigEntry{};
                            break;
                        case OutputConfig::OutputConfigEntryType::kMqtt:
                            output_config.outputs[i] = OutputConfig::MqttOutputConfigEntry{};
                            break;
                        case OutputConfig::OutputConfigEntryType::kImage:
                            output_config.outputs[i] = OutputConfig::ImageOutputConfigEntry{};
                            break;
                        }
                    }
                }

                std::visit(
                    Overloaded{
                        [&](OutputConfig::CheckpointOutputConfigEntry& checkpoint) {
                            {
                                const char* label = "Static Format";
                                const char* tooltip = "Data format for static spatial maps (e.g., topography DEMs).";

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text("%s", label);

                                ImGui::TableSetColumnIndex(1);
                                ImGui::SetNextItemWidth(std::min(120.0f, ImGui::GetContentRegionAvail().x));
                                EnumComboBox<FileStaticFormat>(label, checkpoint.static_format, tooltip);
                            }
                        },
                        [&](OutputConfig::MqttOutputConfigEntry& mqtt) {
                            {
                                const char* label = "Broker Address";
                                const char* tooltip = "Connection settings for the MQTT broker.";

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text("%s", label);

                                ImGui::TableSetColumnIndex(1);
                                static const std::vector<std::string> mqtt_protocols = { "mqtt://", "mqtts://", "ws://", "wss://" };
                                ServerAddressInput(
                                    label,
                                    mqtt.protocol,
                                    mqtt.host,
                                    mqtt.port,
                                    mqtt_protocols,
                                    std::min(800.0f, ImGui::GetContentRegionAvail().x),
                                    tooltip
                                );
                            }

                            {
                                const char* label = "Payload Format";
                                const char* tooltip = "Format of the published messages.";

                                ImGui::TableNextRow();
                                ImGui::TableSetColumnIndex(0);
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text("%s", label);

                                ImGui::TableSetColumnIndex(1);
                                ImGui::SetNextItemWidth(std::min(120.0f, ImGui::GetContentRegionAvail().x));
                                EnumComboBox<OutputConfig::MqttOutputConfigEntry::PayloadFormat>(label, mqtt.payload_format, tooltip);
                            }
                        },
                        [&](auto&) {
                            ImGui::TableNextRow();
                            ImGui::TableSetColumnIndex(0);
                            ImGui::Text("Settings");

                            ImGui::TableSetColumnIndex(1);
                            ImGui::TextDisabled("No specific settings for this output type.");
                        }
                    },
                    output_config.outputs[i]
                );

                ImGui::EndTable();
            }

            ImGui::Separator();
            ImGui::Spacing();
            ImGui::PopID();
        }

		// Delete the item safely outside of the ImGui loop to avoid invalidating indices
        if (item_to_delete != -1) {
            output_config.outputs.erase(output_config.outputs.begin() + item_to_delete);
        }

        // --- ADD OUTPUT BUTTON ---
        if (ImGui::Button("+ Add Output", ImVec2(-FLT_MIN, 0))) {
			// Add a default output
            output_config.outputs.push_back(OutputConfig::ImageOutputConfigEntry{});
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[GUI Error] Exception caught while rendering Output Tab: " << e.what() << "\n";
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "An error occurred rendering the Output Tab.");
    }
    catch (...) {
        std::cerr << "[GUI Error] Unknown exception caught while rendering Output Tab.\n";
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "A critical unknown error occurred rendering the Output Tab.");
    }
}

}  // namespace floodsim::gui
