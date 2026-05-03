
#include "gui/tabs/Tabs.hpp"

namespace FloodSim::Gui {

    // ---------------------------------------------------------------------
    // C++20 Visitor Helper (Igual que en StateUpdaterFactory.cpp)
    // ---------------------------------------------------------------------
    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

	void renderStateUpdaterTab(danasim::StateUpdaterConfig& stateUpdaterConfig) {
        ImGui::Spacing();
        ImGui::SeparatorText("General");
        ImGui::Spacing();

        if (ImGui::BeginTable("StateUpdaterGeneralTable", 2, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

            {
                const char* label = "Enable Rainfall";
                const char* tooltip = "";

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                Checkbox(label, stateUpdaterConfig.enableRainfall, tooltip);
            }

            {
                const char* label = "Dry Tolerance";
                const char* tooltip = "";

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-FLT_MIN);
                FloatInput(label, stateUpdaterConfig.dryTolerance, tooltip);
            }

            ImGui::EndTable();
        }


        ImGui::Spacing();
        ImGui::SeparatorText("Model Updater");
        ImGui::Spacing();

        if (ImGui::BeginTable("StateUpdaterModelTable", 2, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

            {
                const char* label = "Model Type";
                const char* tooltip = "Select the type of mathematical/AI model for state updating.";

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-FLT_MIN);

                // 1. DEDUCIR EL ENUM DESDE EL VARIANT ACTIVO
                // Leemos qué tipo de estructura hay actualmente en el variant
                danasim::UpdaterConfigType currentType;

                std::visit(overloaded{
                        [&currentType](danasim::OnnxUpdaterConfig&) { currentType = danasim::UpdaterConfigType::ONNX; },
                        // Puedes añadir más lambdas aquí a medida que crees nuevos tipos
                        [](auto&&) {
                            throw std::runtime_error("StateUpdaterTab: Unimplemented updater type.");
                        }
                    },
                    stateUpdaterConfig.updater
                );

                danasim::UpdaterConfigType prevType = currentType;
                EnumComboBox<danasim::UpdaterConfigType>(label, currentType, tooltip);

                if (currentType != prevType) {
                    switch (currentType) {
                    case danasim::UpdaterConfigType::ONNX:
                        stateUpdaterConfig.updater = danasim::OnnxUpdaterConfig{};
                        break;
                        // Añade futuros cases aquí (ej. case UpdaterConfigType::PHYSICS: ...)
                    }
                }
            }


            std::visit(overloaded{

                // --- ONNX Strategy ---
                [](danasim::OnnxUpdaterConfig& onnx) {
                    {
                        const char* label = "Model Path";
                        const char* tooltip = "Directory containing the exported ONNX model.";

                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("%s", label);

                        ImGui::TableSetColumnIndex(1);
                        FolderInput(label, onnx.modelPath, tooltip);
                    }

                    {
                        const char* label = "Tensor Dimension";
                        const char* tooltip = "Size of the N-dimensional tensor.";

                        ImGui::TableNextRow();

                        ImGui::TableSetColumnIndex(0);
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("%s", label);

                        ImGui::TableSetColumnIndex(1);
                        ImGui::SetNextItemWidth(-FLT_MIN);

                        // Definimos los valores exactos permitidos
                        const std::vector<int64_t> allowedTensorDims = { 16, 32, 64, 128, 256, 512, 1024 };

                        // Si el valor actual por defecto no está en la lista, lo forzamos al primero por seguridad
                        if (std::find(allowedTensorDims.begin(), allowedTensorDims.end(), onnx.tensorDim) == allowedTensorDims.end()) {
                            onnx.tensorDim = allowedTensorDims.front();
                        }

                        ComboBox<int64_t>(label, onnx.tensorDim, allowedTensorDims, tooltip);
                    }
                },

                // --- Catch-all (Por si añades un tipo al variant pero olvidas añadir su interfaz) ---
                [](auto&&) {
                    ImGui::TableNextRow();

                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextDisabled("Configuration");

                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextDisabled("No specific settings for this updater type.");
                }

                }, stateUpdaterConfig.updater);

            ImGui::EndTable();
        }


        ImGui::Spacing();
        ImGui::SeparatorText("Flood Risk Levels");
        ImGui::Spacing();

        // --- 1. Nivel por Defecto (Siempre el índice 0) ---
        ImGui::BulletText("Default Level (Threshold: 0.0):");
        ImGui::Spacing();

        if (ImGui::BeginTable("DefaultLevelTable", 2, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, 180.0f);
            ImGui::TableSetupColumn("Input", ImGuiTableColumnFlags_WidthStretch);

            // Verificación de seguridad
            if (stateUpdaterConfig.floodRiskLevels.empty()) {
                stateUpdaterConfig.floodRiskLevels.resize(1);
            }

            auto& defaultLvl = stateUpdaterConfig.floodRiskLevels[0];

            {
                const char* label = "Name";
                const char* tooltip = "Name of the default dry state.";

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-FLT_MIN);

                TextInput(label, defaultLvl.name, tooltip);
            }

            {
                const char* label = "Color (Hex)";
                const char* tooltip = "Base color for dry terrain.";

                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s", label);

                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-FLT_MIN);
                ColorInput(label, defaultLvl.colorHex, tooltip);
            }

            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::BulletText("Dynamic Threshold Levels:");
        ImGui::Spacing();

        if (ImGui::Button(" + Add New Level ")) {
            stateUpdaterConfig.floodRiskLevels.push_back({ "New Level", 0.01f, "FFFFFFFF" });
        }

        ImGui::Spacing();

        // --- 2. Tabla de Niveles Dinámicos ---
        if (ImGui::BeginTable("DynamicLevelsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 2.0f);
            ImGui::TableSetupColumn("Threshold (Start)", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableSetupColumn("Color", ImGuiTableColumnFlags_WidthStretch, 1.5f);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableHeadersRow();

            int itemToDelete = -1;

            for (size_t i = 1; i < stateUpdaterConfig.floodRiskLevels.size(); ++i) {
                auto& level = stateUpdaterConfig.floodRiskLevels[i];

                ImGui::PushID(static_cast<int>(i));

                ImGui::TableNextRow();

                // Columna 1: Nombre
                ImGui::TableSetColumnIndex(0);
                ImGui::SetNextItemWidth(-FLT_MIN);

                {
                    const char* label = "Name";
                    const char* tooltip = "";

                    TextInput(label, level.name, tooltip);
                }

                // Columna 2: Threshold
                ImGui::TableSetColumnIndex(1);
                ImGui::SetNextItemWidth(-FLT_MIN);

                {
                    const char* label = "Threshold";
                    const char* tooltip = "";

                    FloatInput(label, level.thresholdStart, tooltip);

                    if (level.thresholdStart <= 0.0f) level.thresholdStart = 0.0001f;
                }

                // Columna 3: Color Inline (¡Reutilización máxima del helper de bajo nivel!)
                ImGui::TableSetColumnIndex(2);
                ImGui::SetNextItemWidth(-FLT_MIN);

                {
                    const char* label = "Color";
                    const char* tooltip = "";

                    ColorInput(label, level.colorHex, tooltip);
                }

                // Columna 4: Botón de Borrar
                ImGui::TableSetColumnIndex(3);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                if (ImGui::Button("Remove", ImVec2(-FLT_MIN, 0))) {
                    itemToDelete = static_cast<int>(i);
                }
                ImGui::PopStyleColor(2);

                ImGui::PopID();
            }

            ImGui::EndTable();

            if (itemToDelete >= 1) {
                stateUpdaterConfig.floodRiskLevels.erase(stateUpdaterConfig.floodRiskLevels.begin() + itemToDelete);
            }
        }

        // --- 3. ORDENACIÓN AUTOMÁTICA ---
        // Ordenamos la lista basándonos en el threshold, PERO empezando desde el elemento 1
        // Esto asegura que el elemento 0 (Default) nunca se mueva ni se pierda.

        // Solo ordenamos si NO hay ningún elemento activo siendo editado por el usuario.
        if (!ImGui::IsAnyItemActive() && stateUpdaterConfig.floodRiskLevels.size() > 1) {
            std::sort(stateUpdaterConfig.floodRiskLevels.begin() + 1, stateUpdaterConfig.floodRiskLevels.end(),
                [](const auto& a, const auto& b) {
                    return a.thresholdStart < b.thresholdStart;
                });
        }
	}

}
