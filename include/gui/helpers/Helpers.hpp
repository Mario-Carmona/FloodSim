/**
 * \file Helpers.hpp
 * \brief ImGui widget wrappers and helper functions for the GUI layer.
 *
 * Provides standardized, safely-wrapped ImGui controls for inputs, dates,
 * colors, and enumerations.
 */

#ifndef FLOODSIM_GUI_HELPERS_HPP_
#define FLOODSIM_GUI_HELPERS_HPP_

#include <chrono>
#include <ctime>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <magic_enum/magic_enum.hpp>

namespace floodsim::gui {

    /**
    * \brief Standard text input field.
    * \param label The identifier and display name of the widget.
    * \param value The string to modify.
    * \param tooltip Optional tooltip displayed on hover.
    */
    void TextInput(const char* label, std::string& value, const char* tooltip = nullptr);

    /**
    * \brief Standard float input field.
    * \param label The identifier and display name of the widget.
    * \param value The float value to modify.
    * \param tooltip Optional tooltip displayed on hover.
    */
    void FloatInput(const char* label, float& value, const char* tooltip = nullptr);

    /**
    * \brief Standard double precision input field.
    * \param label The identifier and display name of the widget.
    * \param value The double value to modify.
    * \param tooltip Optional tooltip displayed on hover.
    */
    void DoubleInput(const char* label, double& value, const char* tooltip = nullptr);

    /**
    * \brief Standard 64-bit integer input field.
    * \param label The identifier and display name of the widget.
    * \param value The 64-bit integer value to modify.
    * \param tooltip Optional tooltip displayed on hover.
    */
    void Int64Input(const char* label, int64_t& value, const char* tooltip = nullptr);

    /**
    * \brief Standard checkbox for boolean values.
    * \param label The identifier and display name of the widget.
    * \param value The boolean value to modify.
    * \param tooltip Optional tooltip displayed on hover.
    */
    void Checkbox(const char* label, bool& value, const char* tooltip = nullptr);

    /**
    * \brief Folder selection input triggering a system file dialog.
    * \param label The identifier and display name of the widget.
    * \param value The path object to populate with the selected directory.
    * \param item_width The width of the input field.
    * \param tooltip Optional tooltip displayed on hover.
    */
    void FolderInput(const char* label, std::filesystem::path& value, float item_width, const char* tooltip = nullptr);

    /**
    * \brief Color picker that reads and writes a HEX color string.
    * \param label The identifier and display name of the widget.
    * \param hex_color The HEX string (e.g., "FF0000" or "FF0000FF") to modify.
    * \param tooltip Optional tooltip displayed on hover.
    * \param extra_flags Additional ImGui color edit flags.
    */
    void ColorInput(const char* label, std::string& hex_color, const char* tooltip = nullptr, ImGuiColorEditFlags extra_flags = 0);

    /**
    * \brief Selects both Date and Time combined into a system timestamp.
    * \param label The identifier and display name of the widget.
    * \param timestamp The chrono timestamp to modify.
    * \param tooltip Optional tooltip displayed on hover.
    */
    void TimestampPicker(const char* label, std::chrono::sys_seconds& timestamp, const char* tooltip = nullptr);

    /**
    * \brief Selects ONLY the Date (maintains internal time intact).
    * \param label The identifier and display name of the widget.
    * \param date The chrono timestamp to modify.
    * \param tooltip Optional tooltip displayed on hover.
    */
    void DatePicker(const char* label, std::chrono::sys_seconds& date, const char* tooltip = nullptr);

    /**
    * \brief Selects ONLY the Time (maintains internal date intact).
    * \param label The identifier and display name of the widget.
    * \param time The chrono duration to modify.
    * \param tooltip Optional tooltip displayed on hover.
    */
    void TimePicker(const char* label, std::chrono::seconds& time, const char* tooltip = nullptr);

    /**
    * \brief Compound input field for Server Address (Protocol, Host, Port).
    * \param label The identifier and display name of the widget.
    * \param protocol The protocol string to modify.
    * \param host The host string to modify.
    * \param port The port integer to modify.
    * \param protocols A list of valid protocols.
    * \param item_width Total width of the combined widget.
    * \param tooltip Optional tooltip displayed on hover.
    */
    void ServerAddressInput(const char* label, std::string& protocol, std::string& host, int& port, const std::vector<std::string>& protocols, float item_width, const char* tooltip = nullptr);

    /**
    * \brief A generic combo box for standard types (strings, numerics).
    * \tparam T The type of the value being selected.
    * \param label The identifier and display name of the widget.
    * \param value The value to modify.
    * \param options A vector of selectable options.
    * \param tooltip Optional tooltip displayed on hover.
    */
    template <typename T>
    void ComboBox(const char* label, T& value, const std::vector<T>& options, const char* tooltip = nullptr) {
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

        std::string preview_value = get_as_text(value);

        if (ImGui::BeginCombo("##combo", preview_value.c_str())) {
            for (const T& val : options) {
                bool is_selected = (value == val);
                std::string item_text = get_as_text(val);

                if (ImGui::Selectable(item_text.c_str(), is_selected)) {
                    value = val;
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
    * \brief A combo box strictly typed for enumerations, utilizing magic_enum.
    * \tparam T The enum type.
    * \param label The identifier and display name of the widget.
    * \param value The enum value to modify.
    * \param tooltip Optional tooltip displayed on hover.
    */
    template <typename T>
    void EnumComboBox(const char* label, T& value, const char* tooltip = nullptr) {
        static_assert(std::is_enum_v<T>, "Error: Type parameter must be an Enum.");

        std::string value_str(magic_enum::enum_name(value));
        constexpr auto names_array = magic_enum::enum_names<T>();

        std::vector<std::string> options;
        options.reserve(names_array.size());
        for (const auto& name : names_array) {
            options.emplace_back(name);
        }

        ComboBox<std::string>(label, value_str, options, tooltip);

        if (auto parsed = magic_enum::enum_cast<T>(value_str, magic_enum::case_insensitive)) {
            value = parsed.value();
        }
    }

}  // namespace floodsim::gui

#endif  // FLOODSIM_GUI_HELPERS_HPP_
