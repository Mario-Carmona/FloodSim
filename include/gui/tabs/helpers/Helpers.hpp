/**
 * @file Helpers.hpp
 * @brief ImGui widget wrappers and helper functions for the GUI layer.
 *
 * Provides standardized, safely-wrapped ImGui controls for inputs, dates,
 * colors, and enumerations with automated prefix stripping for enums.
 * 
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <chrono>
#include <ctime>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>
#include <cctype>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <magic_enum/magic_enum.hpp>

namespace floodsim::gui {

/**
 * @brief Standard text input field.
 * @param label The identifier and display name of the widget.
 * @param value The string to modify.
 * @param tooltip Optional tooltip displayed on hover.
 */
void TextInput(const char* label, std::string& value, const char* tooltip = nullptr);

/**
 * @brief Standard float input field.
 * @param label The identifier and display name of the widget.
 * @param value The float value to modify.
 * @param tooltip Optional tooltip displayed on hover.
 */
void FloatInput(const char* label, float& value, const char* tooltip = nullptr);

/**
 * @brief Standard double precision input field.
 * @param label The identifier and display name of the widget.
 * @param value The double value to modify.
 * @param tooltip Optional tooltip displayed on hover.
 */
void DoubleInput(const char* label, double& value, const char* tooltip = nullptr);

/**
 * @brief Standard 64-bit integer input field.
 * @param label The identifier and display name of the widget.
 * @param value The 64-bit integer value to modify.
 * @param tooltip Optional tooltip displayed on hover.
 */
void Int64Input(const char* label, int64_t& value, const char* tooltip = nullptr);

/**
 * @brief Standard boolean checkbox.
 * @param label The identifier and display name of the widget.
 * @param value The boolean value to modify.
 * @param tooltip Optional tooltip displayed on hover.
 */
void Checkbox(const char* label, bool& value, const char* tooltip = nullptr);

/**
 * @brief Server address input fields (protocol, host, port).
 * @param label Identifier for the input block.
 * @param protocol String storing the current protocol.
 * @param host String storing the server address/host.
 * @param port Integer storing the port number.
 * @param protocols List of available protocols to display in the dropdown.
 * @param item_width Total width allocated for this input block.
 * @param tooltip Optional tooltip displayed on hover.
 */
void ServerAddressInput(const char* label, std::string& protocol, std::string& host, int& port,
                        const std::vector<std::string>& protocols, float item_width, const char* tooltip = nullptr);

/**
 * @brief Folder selection input triggering a system file dialog.
 * @param label The identifier and display name of the widget.
 * @param path The path variable to modify via the interactive dialog.
 * @param width The width of the input field.
 * @param tooltip Optional tooltip displayed on hover.
 */
void FolderInput(const char* label, std::filesystem::path& path, float width, const char* tooltip = nullptr);

/**
 * @brief Generic combo box for string selection.
 * @tparam T Type of the elements (usually std::string).
 * @param label Display name.
 * @param current_value The currently selected value.
 * @param options List of available options.
 * @param tooltip Optional tooltip.
 */
template <typename T>
void ComboBox(const char* label, T& current_value, const std::vector<T>& options, const char* tooltip = nullptr) {
    if (!label) return;
    ImGui::PushID(label);

    auto get_as_text = [](const T& val) -> std::string {
        try {
            if constexpr (std::is_same_v<T, std::string>) {
                return val;
            }
            else if constexpr (std::is_arithmetic_v<T>) {
                return std::to_string(val);
            }
            else {
                static_assert(std::is_same_v<T, std::string> || std::is_arithmetic_v<T>,
                    "ComboBox only supports std::string or numeric types.");
                return "Unknown";
            }
        }
        catch (const std::exception& e) {
            // Failsafe string generation in case of conversion errors
            return "Error";
        }
    };

    std::string preview_value = get_as_text(current_value);

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::BeginCombo("##Combo", preview_value.c_str())) {
        for (const auto& item : options) {
            bool is_selected = (current_value == item);
            std::string item_text = get_as_text(item);
            if (ImGui::Selectable(item_text.c_str(), is_selected)) {
                current_value = item;
            }
            if (is_selected) {
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

/**
 * @brief Utility to strip Google Style 'k' prefix from enum names for display.
 * @param raw_name The raw name generated by magic_enum (e.g., "kMqtt").
 * @return A clean string for the UI (e.g., "Mqtt").
 */
inline std::string CleanEnumDisplayName(std::string_view raw_name) {
    if (raw_name.size() > 1 && raw_name[0] == 'k' && std::isupper(static_cast<unsigned char>(raw_name[1]))) {
        return std::string(raw_name.substr(1));
    }
    return std::string(raw_name);
}

/**
 * @brief A combo box strictly typed for enumerations, utilizing magic_enum.
 * @tparam T The enum type.
 * @param label The identifier and display name of the widget.
 * @param value The enum value to modify.
 * @param tooltip Optional tooltip displayed on hover.
 */
template <typename T>
void EnumComboBox(const char* label, T& value, const char* tooltip = nullptr) {
    static_assert(std::is_enum_v<T>, "Error: Type parameter must be an Enum.");

    if (!label) return;
    ImGui::PushID(label);

    std::string current_display = CleanEnumDisplayName(magic_enum::enum_name(value));
    constexpr auto entries = magic_enum::enum_entries<T>();

    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    if (ImGui::BeginCombo("##EnumCombo", current_display.c_str())) {
        for (const auto& [enum_val, raw_name] : entries) {
            std::string clean_name = CleanEnumDisplayName(raw_name);
            bool is_selected = (value == enum_val);

            if (ImGui::Selectable(clean_name.c_str(), is_selected)) {
                value = enum_val;
            }

            if (is_selected) {
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

/**
 * @brief Color picker input field binding directly to a hexadecimal string.
 * @param label The identifier and display name of the widget.
 * @param hex_color Hexadecimal string (e.g., "FF0000" or "#FF0000FF") to modify.
 * @param tooltip Optional tooltip displayed on hover.
 * @param extra_flags Additional ImGui color edit flags.
 */
void ColorInput(const char* label, std::string& hex_color, const char* tooltip = nullptr, ImGuiColorEditFlags extra_flags = 0);

/**
 * @brief Selects both Date and Time combined into a system timestamp.
 * @param label The identifier and display name of the widget.
 * @param timestamp The chrono timestamp to modify.
 * @param tooltip Optional tooltip displayed on hover.
 */
void TimestampPicker(const char* label, std::chrono::sys_seconds& timestamp, const char* tooltip = nullptr);

/**
 * @brief Selects ONLY the Date (maintains internal time intact).
 * @param label The identifier and display name of the widget.
 * @param date The chrono timestamp to modify.
 * @param tooltip Optional tooltip displayed on hover.
 */
void DatePicker(const char* label, std::chrono::sys_seconds& date, const char* tooltip = nullptr);

/**
 * @brief Selects ONLY the Time (maintains internal date intact).
 * @param label The identifier and display name of the widget.
 * @param time The chrono duration to modify.
 * @param tooltip Optional tooltip displayed on hover.
 */
void TimePicker(const char* label, std::chrono::seconds& time, const char* tooltip = nullptr);

}  // namespace floodsim::gui
