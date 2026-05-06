
#include "gui/tabs/Tabs.hpp"

#include <type_traits>

namespace FloodSim::Gui {

    // ---------------------------------------------------------------------
    // C++20 Visitor Helper (Igual que en StateUpdaterFactory.cpp)
    // ---------------------------------------------------------------------
    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

	void renderOutputTab(danasim::OutputConfig& outputConfig) {
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
                ImGui::SetNextItemWidth(std::min(200.0f, ImGui::GetContentRegionAvail().x));
                Int64Input(label, outputConfig.snapshot.everyNSteps, tooltip);
                if (outputConfig.snapshot.everyNSteps < 1)  outputConfig.snapshot.everyNSteps = 1;
            }

            ImGui::EndTable();
        }


        // ==========================================
        // 2. OUTPUTS LIST
        // ==========================================
        ImGui::Spacing();
        ImGui::SeparatorText("Simulation Outputs");
        ImGui::Spacing();

        // Bucle a través de la lista de outputs (std::vector<std::variant>)
        for (size_t i = 0; i < outputConfig.outputs.size(); ++i) {
            ImGui::PushID(static_cast<int>(i)); // CRÍTICO: Evita colisiones entre outputs

            // --- Cabecera del Output y Botón Remove ---
            ImGui::AlignTextToFramePadding();
            ImGui::BulletText("Output #%zu", i + 1);

            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));

            if (ImGui::Button("Remove", ImVec2(80.0f, 0))) {
                // Si el usuario borra, eliminamos el elemento, ajustamos el loop y pasamos al siguiente
                outputConfig.outputs.erase(outputConfig.outputs.begin() + i);
                ImGui::PopID();
                --i;
                continue;
            }

            ImGui::PopStyleColor(2);

            ImGui::Spacing();

            // --- Formulario Específico ---
            if (ImGui::BeginTable("OutputItemTable", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

                {
                    const char* label = "Output Type";
                    const char* tooltip = "Select the format for this output.";

                    // 1. SELECTOR DE TIPO (Común a todos)
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text(label);

                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(std::min(120.0f, ImGui::GetContentRegionAvail().x));

                    danasim::OutputConfig::OutputConfigEntryType currentType;

                    std::visit(overloaded{
                            [&currentType](danasim::OutputConfig::X3dFileOutputConfigEntry&) { currentType = danasim::OutputConfig::OutputConfigEntryType::X3D_FILE; },
                            [&currentType](danasim::OutputConfig::MqttOutputConfigEntry&) { currentType = danasim::OutputConfig::OutputConfigEntryType::MQTT; },
                            [&currentType](danasim::OutputConfig::ImageOutputConfigEntry&) { currentType = danasim::OutputConfig::OutputConfigEntryType::IMAGE; },
                            // Puedes añadir más lambdas aquí a medida que crees nuevos tipos
                            [](auto&&) {
                                throw std::runtime_error("OutputTab: Unimplemented output type.");
                            }
                        },
                        outputConfig.outputs[i]
                    );

                    danasim::OutputConfig::OutputConfigEntryType prevType = currentType;
                    EnumComboBox<danasim::OutputConfig::OutputConfigEntryType>(label, currentType, tooltip);

                    if (currentType != prevType) {
                        switch (currentType) {
                        case danasim::OutputConfig::OutputConfigEntryType::X3D_FILE:
                            outputConfig.outputs[i] = danasim::OutputConfig::X3dFileOutputConfigEntry{};
                            break;
                        case danasim::OutputConfig::OutputConfigEntryType::MQTT:
                            outputConfig.outputs[i] = danasim::OutputConfig::MqttOutputConfigEntry{};
                            break;
                        case danasim::OutputConfig::OutputConfigEntryType::IMAGE:
                            outputConfig.outputs[i] = danasim::OutputConfig::ImageOutputConfigEntry{};
                            break;
                        }
                    }
                }


                std::visit(overloaded{

                    [](danasim::OutputConfig::MqttOutputConfigEntry& specificOutput) {
                        // --- Address ---
                        {
                            const char* label = "Broker Address";
                            const char* tooltip = "Connection settings for the MQTT broker.";

                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("%s", label);

                            ImGui::TableSetColumnIndex(1);
                            
                            // Definimos qué protocolos permitimos para este input concreto
                            static const std::vector<std::string> mqttProtocols = { "mqtt://", "mqtts://", "ws://", "wss://" };

                            // Llamamos a nuestro potente Helper
                            ServerAddressInput(
                                label,
                                specificOutput.protocol,
                                specificOutput.host,
                                specificOutput.port,
                                mqttProtocols,
                                std::min(800.0f, ImGui::GetContentRegionAvail().x),
                                tooltip
                            );
                        }

                        // --- Format ---
                        {
                            const char* label = "Payload Format";
                            const char* tooltip = "Format of the published messages.";

                            ImGui::TableNextRow();

                            ImGui::TableSetColumnIndex(0);
                            ImGui::AlignTextToFramePadding();
                            ImGui::Text("%s", label);

                            ImGui::TableSetColumnIndex(1);
                            ImGui::SetNextItemWidth(std::min(120.0f, ImGui::GetContentRegionAvail().x));

                            // NOTA: Si 'format' es un std::string usamos TextInput.
                            // Si lo declaraste como un enum, cambia esta línea por:
                            // EnumComboBox("##MqttFormat", specificOutput.format, tooltip);
                            EnumComboBox<danasim::OutputConfig::MqttOutputConfigEntry::PayloadFormat>(label, specificOutput.payloadFormat, tooltip);
                        }
                    },

                    // --- Catch-all (Por si añades un tipo al variant pero olvidas añadir su interfaz) ---
                    [](auto&&) {
                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);
                        ImGui::TextDisabled("Configuration");

                        ImGui::TableSetColumnIndex(1);
                        ImGui::TextDisabled("No specific settings for this output type.");
                    }

                    }, 
                    outputConfig.outputs[i]
                );

                ImGui::EndTable();
            }

            ImGui::Separator();
            ImGui::Spacing();
            ImGui::PopID(); // Fin del bloque de este output
        }

        // --- Botón para Añadir un Nuevo Output ---
        if (ImGui::Button("Add Output", ImVec2(-FLT_MIN, 0))) {
            // Añadimos por defecto un output de tipo Image al final de la lista
            outputConfig.outputs.emplace_back();
        }
	}

}
