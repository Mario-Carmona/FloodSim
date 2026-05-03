
#pragma once

#include <vector>
#include <string>
#include <filesystem>
#include <imgui.h>
#include <imgui_stdlib.h>
#include <portable-file-dialogs.h>
#include <type_traits>
#include <chrono>
#include <ctime>

#include <magic_enum/magic_enum.hpp>

namespace FloodSim::Gui {

    // Helper para campos de texto estándar
    void TextInput(const char* label, std::string& value, const char* tooltip = nullptr);

    void FloatInput(const char* label, float& value, const char* tooltip = nullptr);

    void DoubleInput(const char* label, double& value, const char* tooltip = nullptr);

    void Int64Input(const char* label, int64_t& value, const char* tooltip = nullptr);

    // Helper para booleanos (Checkbox)
    void Checkbox(const char* label, bool& value, const char* tooltip = nullptr);

    // Helper para selección de carpetas
    void FolderInput(const char* label, std::filesystem::path& value, const char* tooltip = nullptr);

    void ColorInput(const char* label, std::string& hexColor, const char* tooltip = nullptr, ImGuiColorEditFlags extraFlags = 0);

    // Selecciona Fecha y Hora combinadas
    void TimestampPicker(const char* label, std::chrono::sys_seconds& timestamp, const char* tooltip = nullptr);

    // Selecciona SOLO la Fecha (Mantiene la hora intacta internamente)
    void DatePicker(const char* label, std::chrono::sys_seconds& date, const char* tooltip = nullptr);

    // Selecciona SOLO la Hora (Mantiene la fecha intacta internamente)
    void TimePicker(const char* label, std::chrono::seconds& time, const char* tooltip = nullptr);

    void ServerAddressInput(const char* label, std::string& protocol, std::string& host, int& port, const std::vector<std::string>& protocols, const char* tooltip = nullptr);

    template <typename T>
    void ComboBox(const char* label, T& value, const std::vector<T>& options, const char* tooltip = nullptr) {
        ImGui::PushID(label);

        // --- MAGIA C++17: Función lambda para convertir de forma segura a string ---
        auto getAsText = [](const T& val) -> std::string {
            if constexpr (std::is_same_v<T, std::string>) {
                return val; // Si ya es un string, lo devolvemos tal cual
            }
            else if constexpr (std::is_arithmetic_v<T>) {
                return std::to_string(val); // Si es un número (int, float...), usamos to_string
            }
            else {
                // Si intentas pasar un struct o clase no soportado, el compilador avisará aquí
                static_assert(std::is_same_v<T, std::string> || std::is_arithmetic_v<T>,
                    "FormComboBox solo soporta std::string o tipos numéricos.");
                return "Unknown";
            }
        };

        // Convertimos el valor actual a string para mostrarlo en la caja cerrada
        std::string previewValue = getAsText(value);

        // Iniciamos el ComboBox
        if (ImGui::BeginCombo("##combo", previewValue.c_str())) {
            for (const T& val : options) {
                // Comprobamos si este elemento es el que está seleccionado actualmente
                bool isSelected = (value == val);

                // Convertimos la opción a string para el menú
                std::string itemText = getAsText(val);

                if (ImGui::Selectable(itemText.c_str(), isSelected)) {
                    value = val; // Si el usuario hace clic, actualizamos la variable real
                }

                // Ponemos el foco en el elemento seleccionado si el menú acaba de abrirse
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (tooltip && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }

        ImGui::PopID();
    }
    
    template <typename T>
    void EnumComboBox(const char* label, T& value, const char* tooltip = nullptr) {
        // 2. Validación estática (Solo compilará si T es un enum)
        static_assert(std::is_enum_v<T>, "Error: El parámetro debe ser un Enum.");

        std::string valueStr(magic_enum::enum_name(value));

        constexpr auto names_array = magic_enum::enum_names<T>();
        const std::vector<std::string> options(names_array.begin(), names_array.end());

        ComboBox<std::string>(label, valueStr, options, tooltip);

        if (auto parsed = magic_enum::enum_cast<T>(valueStr, magic_enum::case_insensitive)) {
            value = parsed.value();
        }
    }
}
