
#include "gui/tabs/Tabs.hpp"

#include "adapters/input/readers/file/static/FileStaticFormat.hpp"
#include "adapters/input/readers/file/dynamic/FileDynamicFormat.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <unordered_set>
#include <ranges>

namespace FloodSim::Gui {

    static void extractScalars(const danasim::UpdaterConfig& updaterConfig, danasim::InputConfig& inputConfig, std::string& lastLoadedModelPath) {
        std::string currentModelPath;
        
        std::visit([&currentModelPath](const auto& cfg) {
            using T = std::decay_t<decltype(cfg)>;
            // Verificamos si la configuración actual es la de ONNX
            if constexpr (std::is_same_v<T, danasim::OnnxUpdaterConfig>) {
                currentModelPath = cfg.modelPath.string();
            }
            }, 
            updaterConfig
        );


        if (!currentModelPath.empty() && currentModelPath != lastLoadedModelPath) {
            lastLoadedModelPath = currentModelPath;
            std::unordered_set<std::string> expectedScalars;
            
            std::visit([&currentModelPath, &expectedScalars](const auto& cfg) {
                using T = std::decay_t<decltype(cfg)>;
                // Verificamos si la configuración actual es la de ONNX
                if constexpr (std::is_same_v<T, danasim::OnnxUpdaterConfig>) {
                    std::filesystem::path metaPath = std::filesystem::path(currentModelPath) / "metadata.json";

                    if (std::filesystem::exists(metaPath)) {
                        try {
                            std::ifstream file(metaPath);
                            nlohmann::json data = nlohmann::json::parse(file);

							for (const char* method : { "preprocess", "step" }) {
                                for (const auto& input : data[method]["inputs"]) {
                                    // La regla de oro: Si el shape está vacío, es un escalar
                                    if (input["shape"].empty()) {
                                        std::string scalarName = input["name"];
                                        expectedScalars.insert(scalarName);
                                    }
                                }
                            }
                        }
                        catch (const std::exception& e) {
                            // Aquí podrías usar tu LOG_ERROR si algo falla al parsear
                        }
                    }
                }
                },
                updaterConfig
            );

            for (const char* implicitScalar : { "delta_x", "delta_t" }) {
                expectedScalars.erase(implicitScalar);
            }

            std::unordered_map<std::string, std::string> newScalars;

			for (const std::string& scalar : expectedScalars) {
                // Si el escalar ya existía, conservamos su valor, si no, lo inicializamos a "0.0"
                newScalars[scalar] = inputConfig.scalars.count(scalar) ? inputConfig.scalars[scalar] : "0.0";
            }

            inputConfig.scalars = std::move(newScalars);
        }
    }

	void renderInputTab(danasim::InputConfig& inputConfig, const danasim::UpdaterConfig& updaterConfig) {
        ImGui::Spacing();

        // --- SECCIÓN 1: CONFIGURACIÓN DE ARCHIVOS ---
        ImGui::SeparatorText("File Configuration");
        ImGui::Spacing();

        if (ImGui::BeginTable("InputFileConfigTable", 2, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

            {
                const char* label = "Static Format";
                const char* tooltip = "Data format for static spatial maps (e.g., topography DEMs).";

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(std::min(120.0f, ImGui::GetContentRegionAvail().x));
                EnumComboBox<FileStaticFormat>(label, inputConfig.file.staticFormat, tooltip);
            }

            {
                const char* label = "Dynamic Format";
                const char* tooltip = "Data format for dynamic time-series (e.g., rainfall over time).";

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(std::min(120.0f, ImGui::GetContentRegionAvail().x));
                EnumComboBox<FileDynamicFormat>(label, inputConfig.file.dynamicFormat, tooltip);
            }

            {
                const char* label = "Dataset Name";
                const char* tooltip = "Name of the input dataset.";

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(std::min(400.0f, ImGui::GetContentRegionAvail().x));

                TextInput(label, inputConfig.file.datasetName, tooltip);
            }

            ImGui::EndTable();
        }

        ImGui::Spacing();

        // --- SECCIÓN 2: ESCALARES FÍSICOS ---
        ImGui::SeparatorText("Scalars (Auto-detected)");
        ImGui::Spacing();

        static std::string lastLoadedModelPath = "";

        extractScalars(updaterConfig, inputConfig, lastLoadedModelPath);

        if (inputConfig.scalars.empty()) {
            ImGui::TextDisabled("No scalars required.");
        }
        else {
            if (ImGui::BeginTable("InputScalarsTable", 2, ImGuiTableFlags_SizingStretchProp)) {
                ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
                ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

                for (const auto& scalarName : std::views::keys(inputConfig.scalars)) {
                    ImGui::TableNextRow();

                    // Columna 0: Nombre del escalar
                    ImGui::TableSetColumnIndex(0);
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s", scalarName.c_str());

                    // Columna 1: Input de texto (vinculado al mapa de escalares)
                    ImGui::TableSetColumnIndex(1);
                    ImGui::SetNextItemWidth(std::min(200.0f, ImGui::GetContentRegionAvail().x));

                    // Usamos tu Helper (como es un std::unordered_map<string, string>, funciona directo) 
                    TextInput(scalarName.c_str(), inputConfig.scalars[scalarName], "Extracted from model metadata.");
                }
                ImGui::EndTable();
            }
        }
	}

}
