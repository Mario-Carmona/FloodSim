
#include "gui/tabs/Tabs.hpp"

#include <imgui_stdlib.h>
#include <portable-file-dialogs.h>


namespace FloodSim::Gui {

    void renderScenarioTab(danasim::ScenarioConfig& scenarioConfig) {
        ImGui::Spacing();
        // Agrupamos visualmente los campos con un separador de texto
        ImGui::SeparatorText("Basic Information");
        ImGui::Spacing();

        // Creamos una tabla de 2 columnas para nuestro formulario
        // ImGuiTableFlags_BordersInnerV añade una línea sutil entre las columnas (opcional, puedes quitarlo)
        // ImGuiTableFlags_SizingStretchProp hace que las columnas se adapten al ancho de la ventana
        if (ImGui::BeginTable("BasicInfoFormTable", 2, ImGuiTableFlags_SizingStretchProp)) {

            // Configuramos el ancho base de la primera columna para que no ocupe demasiado espacio
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

            {
                const char* label = "Scenario Name";
                const char* tooltip = "Enter a unique name for this simulation run.";

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(std::min(400.0f, ImGui::GetContentRegionAvail().x));
                TextInput(label, scenarioConfig.name, tooltip);
            }

            // Cerramos la tabla
            ImGui::EndTable();
        }


        ImGui::Spacing();
        // Agrupamos visualmente los campos con un separador de texto
        ImGui::SeparatorText("Output Configuration");
        ImGui::Spacing();

        if (ImGui::BeginTable("OutputConfigFormTable", 2, ImGuiTableFlags_SizingStretchProp)) {

            // Configuramos el ancho base de la primera columna para que no ocupe demasiado espacio
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

            {
                const char* label = "Output Directory";
                const char* tooltip = "Select the folder where the simulation results and logs will be saved.";

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                FolderInput(label, scenarioConfig.outputDir, std::min(900.0f, ImGui::GetContentRegionAvail().x), tooltip);
            }

            {
                const char* label = "Append Timestamp";
                const char* tooltip = "If enabled, the simulation start time will be added to the output folder name.";

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);

                Checkbox(label, scenarioConfig.appendStartTimestamp, tooltip);
            }

            // Cerramos la tabla
            ImGui::EndTable();
        }
    }

}
