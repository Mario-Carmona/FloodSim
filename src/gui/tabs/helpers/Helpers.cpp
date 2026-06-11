/**
 * @file Helpers.cpp
 * @brief Implementation of ImGui widget wrappers and helper functions.
 *
 * This file contains the implementation of standardized UI components using ImGui,
 * ensuring safe access, consistent styling, and proper resource management.
 * 
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "gui/tabs/helpers/Helpers.hpp"

#include <cstring>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <exception>

#include <portable-file-dialogs.h>

namespace floodsim::gui {

namespace {

/**
 * @brief Retrieves the number of days in a given month/year accounting for leap years.
 * @param year The year to check.
 * @param month The month to check (1-12).
 * @return The number of days in the specified month.
*/
int GetDaysInMonth(int year, int month) {
    if (month < 1 || month > 12) return 30; // Failsafe
    static constexpr int kDays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (month == 2) {
        bool is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        return is_leap ? 29 : 28;
    }
    return kDays[month - 1];
}

/**
 * @brief Retrieves the string representation of a month.
 * @param month The month number (1-12).
 * @return A C-string containing the 3-letter abbreviation of the month.
 */
const char* GetMonthName(int month) {
    if (month < 1 || month > 12) return "Unknown";
    static constexpr const char* kMonthNames[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    return kMonthNames[month - 1];
}

/**
 * @brief Thread-safe cross-platform conversion from time_t to std::tm.
 * @param t The epoch time to convert.
 * @return A populated std::tm structure.
 */
std::tm GetSafeLocalTime(std::time_t t) {
    std::tm tm_time;
#if defined(_WIN32)
    localtime_s(&tm_time, &t); // Windows secure version
#else
    localtime_r(&t, &tm_time); // POSIX thread-safe version
#endif
    return tm_time;
}

/**
 * @brief Renders the Date Picker UI components without handling chrono conversions.
 * @return True if any value was modified by the user.
 */
bool RenderDatePickerUI(int& year, int& month, int& day, bool& is_hovered) {
    bool changed = false;
    const float kYearWidth = 60.0f;
    const float kMonthWidth = 60.0f;
    const float kDayWidth = 40.0f;

    ImGui::SetNextItemWidth(kDayWidth);
    if (ImGui::DragInt("##Day", &day, 0.2f, 1, 31, "%d")) changed = true;
    is_hovered |= ImGui::IsItemHovered();

    ImGui::SameLine(0, 4.0f);

    ImGui::SetNextItemWidth(kMonthWidth);
    // Asumo que GetMonthName sigue estando en tu namespace anónimo
    if (ImGui::BeginCombo("##Month", GetMonthName(month))) {
        for (int m = 1; m <= 12; ++m) {
            bool is_selected = (month == m);
            if (ImGui::Selectable(GetMonthName(m), is_selected)) {
                month = m;
                changed = true;
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    is_hovered |= ImGui::IsItemHovered();

    ImGui::SameLine(0, 4.0f);

    ImGui::SetNextItemWidth(kYearWidth);
    if (ImGui::DragInt("##Year", &year, 0.5f, 1900, 2100, "%d")) changed = true;
    is_hovered |= ImGui::IsItemHovered();

    return changed;
}

/**
 * @brief Renders the Time Picker UI components without handling chrono conversions.
 * @return True if any value was modified by the user.
 */
bool RenderTimePickerUI(int& hour, int& minute, int& second, bool& is_hovered) {
    bool changed = false;
    const float kTimeUnitWidth = 40.0f;

    ImGui::SetNextItemWidth(kTimeUnitWidth);
    if (ImGui::DragInt("##Hour", &hour, 0.2f, 0, 23, "%02d")) changed = true;
    is_hovered |= ImGui::IsItemHovered();

    ImGui::SameLine(0, 4.0f);
    ImGui::Text(":");
    ImGui::SameLine(0, 4.0f);

    ImGui::SetNextItemWidth(kTimeUnitWidth);
    if (ImGui::DragInt("##Minute", &minute, 0.2f, 0, 59, "%02d")) changed = true;
    is_hovered |= ImGui::IsItemHovered();

    ImGui::SameLine(0, 4.0f);
    ImGui::Text(":");
    ImGui::SameLine(0, 4.0f);

    ImGui::SetNextItemWidth(kTimeUnitWidth);
    if (ImGui::DragInt("##Second", &second, 0.2f, 0, 59, "%02d")) changed = true;
    is_hovered |= ImGui::IsItemHovered();

    return changed;
}

/**
 * @brief Renders the Time Picker UI components supporting fractional seconds.
 * @return True if any value was modified by the user.
 */
bool RenderFractionalTimePickerUI(int& hour, int& minute, double& second, bool& is_hovered) {
    bool changed = false;
    const float kTimeUnitWidth = 40.0f;
    const float kFractionalUnitWidth = 75.0f;

    ImGui::SetNextItemWidth(kTimeUnitWidth);
    if (ImGui::DragInt("##Hour", &hour, 0.2f, 0, 23, "%02d")) changed = true;
    is_hovered |= ImGui::IsItemHovered();

    ImGui::SameLine(0, 4.0f);
    ImGui::Text(":");
    ImGui::SameLine(0, 4.0f);

    ImGui::SetNextItemWidth(kTimeUnitWidth);
    if (ImGui::DragInt("##Minute", &minute, 0.2f, 0, 59, "%02d")) changed = true;
    is_hovered |= ImGui::IsItemHovered();

    ImGui::SameLine(0, 4.0f);
    ImGui::Text(":");
    ImGui::SameLine(0, 4.0f);

    ImGui::SetNextItemWidth(kFractionalUnitWidth);
    if (ImGui::DragScalar("##Second", ImGuiDataType_Double, &second, 0.05f, nullptr, nullptr, "%07.4f")) {
        changed = true;
    }
    is_hovered |= ImGui::IsItemHovered();

    return changed;
}

} // anonymous namespace

void TextInput(const char* label, std::string& value, const char* tooltip) {
    if (!label) return;

    try {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputText(label, &value);
        if (tooltip && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[GUI Error] Exception in TextInput: " << e.what() << '\n';
    }
}

void FloatInput(const char* label, float& value, const char* tooltip) {
    if (!label) return;

    try {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputFloat(label, &value);
        if (tooltip && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[GUI Error] Exception in FloatInput: " << e.what() << '\n';
    }
}

void DoubleInput(const char* label, double& value, const char* tooltip) {
    if (!label) return;

    try {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputDouble(label, &value);
        if (tooltip && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[GUI Error] Exception in DoubleInput: " << e.what() << '\n';
    }
}

void Int64Input(const char* label, int64_t& value, const char* tooltip) {
    if (!label) return;

    try {
        int64_t step = 1;
        int64_t step_fast = 100;
        ImGui::InputScalar("##input", ImGuiDataType_S64, &value, &step, &step_fast);
        if (tooltip && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[GUI Error] Exception in Int64Input: " << e.what() << '\n';
    }
}

void Checkbox(const char* label, bool& value, const char* tooltip) {
    if (!label) return;

    try {
        ImGui::Checkbox(label, &value);
        if (tooltip && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[GUI Error] Exception in Checkbox: " << e.what() << '\n';
    }
}

void FolderInput(const char* label, std::filesystem::path& path, float width, const char* tooltip) {
    if (!label) return;

    try {
        ImGui::PushID(label);

        std::string path_str = path.string();

        ImGui::SetNextItemWidth(width - 40.0f); // Leave room for browse button
        ImGui::InputText("##FolderPath", &path_str, ImGuiInputTextFlags_ReadOnly);

        if (tooltip && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }

        ImGui::SameLine();

        if (ImGui::Button("...", ImVec2(35.0f, 0))) {
            // Using portable-file-dialogs for cross-platform folder dialog
            auto dir = pfd::select_folder("Select Directory", path_str).result();
            if (!dir.empty()) {
                path = std::filesystem::path(dir).lexically_normal();
            }
        }

        ImGui::PopID();
    }
    catch (const std::exception& e) {
        std::cerr << "[GUI Error] Exception in FolderInput: " << e.what() << '\n';
        ImGui::PopID();
    }
}

void ColorInput(const char* label, std::string& hex_color, const char* tooltip, ImGuiColorEditFlags extra_flags) {
    if (!label) return;

    try {
        // 1. Convertir el string hexadecimal a un array float[4] para ImGui
        float col[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        const char* hex_str = hex_color.c_str();

        // Ignorar el '#' si está presente
        if (hex_str[0] == '#') {
            hex_str++;
        }

        size_t len = std::strlen(hex_str);
        if (len == 6 || len == 8) {
            unsigned int r = 0, g = 0, b = 0, a = 255;
            if (len == 6) {
#if defined(_WIN32)
                sscanf_s(hex_str, "%02x%02x%02x", &r, &g, &b);
#else
                std::sscanf(hex_str, "%02x%02x%02x", &r, &g, &b);
#endif
            }
            else {
#if defined(_WIN32)
                sscanf_s(hex_str, "%02x%02x%02x%02x", &r, &g, &b, &a);
#else
                std::sscanf(hex_str, "%02x%02x%02x%02x", &r, &g, &b, &a);
#endif
            }
            col[0] = static_cast<float>(r) / 255.0f;
            col[1] = static_cast<float>(g) / 255.0f;
            col[2] = static_cast<float>(b) / 255.0f;
            col[3] = static_cast<float>(a) / 255.0f;
        }

        // 2. Renderizar el widget ImGui
        if (ImGui::ColorEdit4(label, col, extra_flags)) {
            // 3. Si el usuario modifica el color, actualizar el string original
            char buf[10];
            std::snprintf(buf, sizeof(buf), "#%02X%02X%02X%02X",
                static_cast<unsigned int>(col[0] * 255.0f),
                static_cast<unsigned int>(col[1] * 255.0f),
                static_cast<unsigned int>(col[2] * 255.0f),
                static_cast<unsigned int>(col[3] * 255.0f));
            hex_color = buf;
        }

        if (tooltip && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[GUI Error] Exception in ColorInput: " << e.what() << '\n';
    }
}

void TimestampPicker(const char* label, std::chrono::sys_seconds& timestamp, const char* tooltip) {
    if (!label) return;

    try {
        ImGui::PushID(label);

        std::time_t t_c = std::chrono::system_clock::to_time_t(timestamp);
        std::tm tm_time = GetSafeLocalTime(t_c);

        std::chrono::seconds time_of_day(tm_time.tm_hour * 3600 + tm_time.tm_min * 60 + tm_time.tm_sec);

        DatePicker("##SubDate", timestamp, tooltip);
        ImGui::SameLine(0, 12.0f);
        TimePicker("##SubTime", time_of_day, tooltip);

        // Reconstruir la hora si TimePicker la modificó
        std::time_t t_c_new = std::chrono::system_clock::to_time_t(timestamp);
        tm_time = GetSafeLocalTime(t_c_new);

        auto new_time_count = time_of_day.count();
        tm_time.tm_hour = static_cast<int>((new_time_count / 3600) % 24);
        tm_time.tm_min = static_cast<int>((new_time_count / 60) % 60);
        tm_time.tm_sec = static_cast<int>(new_time_count % 60);
        tm_time.tm_isdst = -1;

        t_c_new = std::mktime(&tm_time);
        if (t_c_new != -1) {
            timestamp = std::chrono::time_point_cast<std::chrono::seconds>(
                std::chrono::system_clock::from_time_t(t_c_new));
        }

        ImGui::PopID();
    }
    catch (const std::exception& e) {
        std::cerr << "[GUI Error] Exception in TimestampPicker: " << e.what() << '\n';
        ImGui::PopID();
    }
}

void DatePicker(const char* label, std::chrono::sys_seconds& date, const char* tooltip) {
    if (!label) return;

    try {
        ImGui::PushID(label);

        std::time_t t_c = std::chrono::system_clock::to_time_t(date);
        std::tm tm_time = GetSafeLocalTime(t_c);

        int year = tm_time.tm_year + 1900;
        int month = tm_time.tm_mon + 1;
        int day = tm_time.tm_mday;
        bool is_hovered = false;

        if (RenderDatePickerUI(year, month, day, is_hovered)) {
            // Validación estricta tras la entrada del usuario
            day = std::clamp(day, 1, GetDaysInMonth(year, month));
            tm_time.tm_year = year - 1900;
            tm_time.tm_mon = month - 1;
            tm_time.tm_mday = day;
            tm_time.tm_isdst = -1; // Mantener configuración local de horario de verano

            t_c = std::mktime(&tm_time);
            if (t_c != -1) {
                date = std::chrono::time_point_cast<std::chrono::seconds>(
                    std::chrono::system_clock::from_time_t(t_c));
            }
        }

        if (tooltip && is_hovered) {
            ImGui::SetTooltip("%s", tooltip);
        }

        ImGui::PopID();
    }
    catch (const std::exception& e) {
        std::cerr << "[GUI Error] Exception in DatePicker: " << e.what() << '\n';
        ImGui::PopID();
    }
}

void TimePicker(const char* label, std::chrono::seconds& time, const char* tooltip) {
    if (!label) return;

    try {
        ImGui::PushID(label);

        auto time_count = time.count();
        int hour = static_cast<int>((time_count / 3600) % 24);
        int minute = static_cast<int>((time_count / 60) % 60);
        int second = static_cast<int>(time_count % 60);
        bool is_hovered = false;

        if (RenderTimePickerUI(hour, minute, second, is_hovered)) {
            time = std::chrono::seconds(hour * 3600 + minute * 60 + second);
        }

        if (tooltip && is_hovered) {
            ImGui::SetTooltip("%s", tooltip);
        }

        ImGui::PopID();
    }
    catch (const std::exception& e) {
        std::cerr << "[GUI Error] Exception in TimePicker: " << e.what() << '\n';
        ImGui::PopID();
    }
}

void FractionalTimePicker(const char* label, std::chrono::duration<double>& time, const char* tooltip) {
    if (!label) return;

    try {
        ImGui::PushID(label);

        // Break down the exact duration into hours, minutes, and fractions of a second
        auto h = std::chrono::duration_cast<std::chrono::hours>(time);
        auto m = std::chrono::duration_cast<std::chrono::minutes>(time - h);
        std::chrono::duration<double> s = time - h - m;

        int hour = static_cast<int>(h.count());
        int minute = static_cast<int>(m.count());
        double second = s.count();
        bool is_hovered = false;

        if (RenderFractionalTimePickerUI(hour, minute, second, is_hovered)) {
            if (second < 0.0) second = 0.0;

            time = std::chrono::hours(hour) +
                std::chrono::minutes(minute) +
                std::chrono::duration<double>(second);
        }

        if (tooltip && is_hovered) {
            ImGui::SetTooltip("%s", tooltip);
        }

        ImGui::PopID();
    }
    catch (const std::exception& e) {
        std::cerr << "[GUI Error] Exception in FractionalTimePicker: " << e.what() << '\n';
        ImGui::PopID();
    }
}

void ServerAddressInput(const char* label, std::string& protocol, std::string& host, int& port, const std::vector<std::string>& protocols, float item_width, const char* tooltip) {
    if (!label) return;

    try {
        ImGui::PushID(label);

        bool is_hovered = false;
        const float kProtocolWidth = 120.0f;
        const float kPortWidth = 100.0f;

        ImGui::SetNextItemWidth(kProtocolWidth);
        if (ImGui::BeginCombo("##Protocol", protocol.c_str())) {
            for (const auto& p : protocols) {
                bool is_selected = (protocol == p);
                if (ImGui::Selectable(p.c_str(), is_selected)) {
                    protocol = p;
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        is_hovered |= ImGui::IsItemHovered();

        ImGui::SameLine(0, 4.0f);

        // Calculate available space dynamically for host text field
        ImGui::SetNextItemWidth(item_width - kProtocolWidth - kPortWidth - 8.0f);
        ImGui::InputText("##Host", &host);
        is_hovered |= ImGui::IsItemHovered();

        ImGui::SameLine(0, 4.0f);

        ImGui::SetNextItemWidth(kPortWidth);
        ImGui::InputInt("##Port", &port, 0, 0); // 0 step means no +/- buttons
        is_hovered |= ImGui::IsItemHovered();

        if (tooltip && is_hovered) {
            ImGui::SetTooltip("%s", tooltip);
        }

        ImGui::PopID();
    }
    catch (const std::exception& e) {
        std::cerr << "[GUI Error] Exception in ServerAddressInput: " << e.what() << '\n';
        // Try to safely pop ID if ImGui state was partially manipulated
        ImGui::PopID();
    }
}

} // namespace floodsim::gui
