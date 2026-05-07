/**
 * \file Helpers.cpp
 * \brief Implementation of ImGui widget wrappers and helper functions.
 */

#include "gui/helpers/Helpers.hpp"

#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <portable-file-dialogs.h>

namespace floodsim::gui {

    namespace {

        /**
            * \brief Retrieves the number of days in a given month/year accounting for leap years.
            */
        int GetDaysInMonth(int year, int month) {
            if (month < 1 || month > 12) return 30; // Failsafe
            static const int kDays[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
            if (month == 2) {
                bool is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
                return is_leap ? 29 : 28;
            }
            return kDays[month - 1];
        }

        /**
            * \brief Retrieves the string representation of a month.
            */
        const char* GetMonthName(int month) {
            if (month < 1 || month > 12) return "Unknown";
            static const char* kNames[] = { "January", "February", "March", "April", "May", "June",
                                            "July", "August", "September", "October", "November", "December" };
            return kNames[month - 1];
        }

        /**
            * \brief Safely extracts local time, falling back to Unix epoch on error.
            */
        std::tm GetSafeLocalTime(std::time_t t) {
            std::tm* tm_ptr = std::localtime(&t);
            std::tm current_tm = {};
            if (tm_ptr != nullptr) {
                current_tm = *tm_ptr;
            }
            else {
                current_tm.tm_year = 70; current_tm.tm_mon = 0; current_tm.tm_mday = 1;
                current_tm.tm_hour = 0;  current_tm.tm_min = 0; current_tm.tm_sec = 0;
            }
            return current_tm;
        }

        /**
            * \brief Renders the Date Picker internal UI component.
            */
        bool RenderDatePickerUI(std::tm& current_tm, int& view_year, int& view_month) {
            bool changed = false;

            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 4));
            if (ImGui::Button("<", ImVec2(24, 0))) {
                view_month -= 1;
                if (view_month < 1) { view_month = 12; view_year -= 1; }
            }
            ImGui::SameLine();

            char month_str[64];
            std::snprintf(month_str, sizeof(month_str), "%s %d", GetMonthName(view_month), view_year);
            float text_width = ImGui::CalcTextSize(month_str).x;
            float center_pos = (ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x - text_width) * 0.5f;

            ImGui::SetCursorPosX(center_pos > 0 ? center_pos : 0.0f);
            ImGui::Text("%s", month_str);

            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - 24);
            if (ImGui::Button(">", ImVec2(24, 0))) {
                view_month += 1;
                if (view_month > 12) { view_month = 1; view_year += 1; }
            }
            ImGui::PopStyleVar();
            ImGui::Separator();

            if (ImGui::BeginTable("DaysTable", 7, ImGuiTableFlags_SizingStretchSame)) {
                const char* days[] = { "Mo", "Tu", "We", "Th", "Fr", "Sa", "Su" };
                for (int i = 0; i < 7; ++i) ImGui::TableSetupColumn(days[i]);
                ImGui::TableHeadersRow();

                std::tm time_in = { 0, 0, 0, 1, view_month - 1, view_year - 1900 };
                std::mktime(&time_in); // Normalize
                int start_day_offset = (time_in.tm_wday + 6) % 7;
                int days_in_month = GetDaysInMonth(view_year, view_month);

                ImGui::TableNextRow();
                for (int i = 0; i < start_day_offset; ++i) {
                    ImGui::TableSetColumnIndex(i);
                    ImGui::Text("");
                }

                for (int day = 1; day <= days_in_month; ++day) {
                    int col = (start_day_offset + day - 1) % 7;
                    ImGui::TableSetColumnIndex(col);

                    char day_str[4];
                    std::snprintf(day_str, sizeof(day_str), "%d", day);

                    bool is_selected_date = (day == current_tm.tm_mday &&
                        view_month == current_tm.tm_mon + 1 &&
                        view_year == current_tm.tm_year + 1900);

                    if (is_selected_date) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
                    }

                    if (ImGui::Button(day_str, ImVec2(-FLT_MIN, 0))) {
                        current_tm.tm_mday = day;
                        current_tm.tm_mon = view_month - 1;
                        current_tm.tm_year = view_year - 1900;
                        changed = true;
                    }

                    if (is_selected_date) ImGui::PopStyleColor();
                    if (col == 6 && day < days_in_month) ImGui::TableNextRow();
                }
                ImGui::EndTable();
            }
            return changed;
        }

        /**
            * \brief Renders the Time Picker internal UI component.
            */
        bool RenderTimePickerUI(std::tm& current_tm) {
            bool changed = false;
            ImGui::TextDisabled("Time (HH:MM:SS)");

            int h = current_tm.tm_hour;
            int m = current_tm.tm_min;
            int s = current_tm.tm_sec;

            auto time_combo_box = [](const char* id, int& val, int max_val) -> bool {
                bool local_changed = false;
                char preview[8];
                std::snprintf(preview, sizeof(preview), "%02d", val);

                ImGui::PushItemWidth(50.0f);
                if (ImGui::BeginCombo(id, preview, ImGuiComboFlags_HeightLargest)) {
                    for (int i = 0; i <= max_val; ++i) {
                        char item[8];
                        std::snprintf(item, sizeof(item), "%02d", i);
                        bool is_selected = (val == i);
                        if (ImGui::Selectable(item, is_selected)) {
                            val = i;
                            local_changed = true;
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();
                return local_changed;
                };

            if (time_combo_box("##h", h, 23)) changed = true;
            ImGui::SameLine(0, 4.0f); ImGui::Text(":"); ImGui::SameLine(0, 4.0f);
            if (time_combo_box("##m", m, 59)) changed = true;
            ImGui::SameLine(0, 4.0f); ImGui::Text(":"); ImGui::SameLine(0, 4.0f);
            if (time_combo_box("##s", s, 59)) changed = true;

            if (changed) {
                current_tm.tm_hour = h;
                current_tm.tm_min = m;
                current_tm.tm_sec = s;
            }

            return changed;
        }

    } // anonymous namespace


    void TextInput(const char* label, std::string& value, const char* tooltip) {
        if (!label) return;
        ImGui::PushID(label);
        ImGui::InputText("##input", &value);
        if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
        ImGui::PopID();
    }

    void FloatInput(const char* label, float& value, const char* tooltip) {
        if (!label) return;
        ImGui::PushID(label);
        ImGui::InputFloat("##input", &value, 0.0f, 0.0f, "%.7g");
        if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
        ImGui::PopID();
    }

    void DoubleInput(const char* label, double& value, const char* tooltip) {
        if (!label) return;
        ImGui::PushID(label);
        ImGui::InputDouble("##input", &value, 0.0, 0.0, "%.8g");
        if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
        ImGui::PopID();
    }

    void Int64Input(const char* label, int64_t& value, const char* tooltip) {
        if (!label) return;
        ImGui::PushID(label);
        int64_t step = 1;
        int64_t step_fast = 100;
        ImGui::InputScalar("##input", ImGuiDataType_S64, &value, &step, &step_fast);
        if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
        ImGui::PopID();
    }

    void Checkbox(const char* label, bool& value, const char* tooltip) {
        if (!label) return;
        ImGui::PushID(label);
        ImGui::Checkbox("##checkbox", &value);
        if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
        ImGui::PopID();
    }

    void FolderInput(const char* label, std::filesystem::path& value, float item_width, const char* tooltip) {
        if (!label) return;
        ImGui::PushID(label);

        std::string temp_str = value.string();
        float browse_button_width = 80.0f;

        if (ImGui::Button("Browse", ImVec2(browse_button_width, 0))) {
            try {
                auto folder = pfd::select_folder("Select Directory", temp_str).result();
                if (!folder.empty()) {
                    value = folder;
                }
            }
            catch (const std::exception& e) {
                std::cerr << "[Error] Folder selection failed: " << e.what() << "\n";
            }
        }

        if (tooltip && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(item_width - browse_button_width);
        ImGui::InputText("##input", &temp_str, ImGuiInputTextFlags_ReadOnly);
        ImGui::PopID();
    }

    void ColorInput(const char* label, std::string& hex_color, const char* tooltip, ImGuiColorEditFlags extra_flags) {
        if (!label) return;
        ImGui::PushID(label);

        unsigned int r = 255, g = 255, b = 255, a = 255;

        // Safely parse hex string, verifying sscanf return bounds
        if (hex_color.length() >= 6) {
            int parsed = std::sscanf(hex_color.c_str(), "%02x%02x%02x", &r, &g, &b);
            if (parsed != 3) { r = g = b = 255; }
        }
        if (hex_color.length() >= 8) {
            int parsed = std::sscanf(hex_color.c_str() + 6, "%02x", &a);
            if (parsed != 1) { a = 255; }
        }

        float col[4] = { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_AlphaPreview | extra_flags;

        if (ImGui::ColorEdit4("##ColorPicker", col, flags)) {
            char buffer[10];
            int written = std::snprintf(buffer, sizeof(buffer), "%02X%02X%02X%02X",
                static_cast<int>(col[0] * 255.0f), static_cast<int>(col[1] * 255.0f),
                static_cast<int>(col[2] * 255.0f), static_cast<int>(col[3] * 255.0f));

            if (written > 0 && written < static_cast<int>(sizeof(buffer))) {
                hex_color = buffer;
            }
        }

        if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);
        ImGui::PopID();
    }

    void TimestampPicker(const char* label, std::chrono::sys_seconds& timestamp, const char* tooltip) {
        if (!label) return;
        ImGui::PushID(label);

        std::time_t t = timestamp.time_since_epoch().count();
        if (t < 0) {
            t = 0;
            timestamp = std::chrono::sys_seconds(std::chrono::seconds(0));
        }
        std::tm current_tm = GetSafeLocalTime(t);

        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
            current_tm.tm_year + 1900, current_tm.tm_mon + 1, current_tm.tm_mday,
            current_tm.tm_hour, current_tm.tm_min, current_tm.tm_sec);

        if (ImGui::Button(buffer, ImVec2(ImGui::CalcItemWidth(), 0))) {
            ImGui::OpenPopup("TimestampPopup");
        }
        if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);

        if (ImGui::BeginPopup("TimestampPopup")) {
            ImGuiStorage* storage = ImGui::GetStateStorage();
            int* view_year = storage->GetIntRef(ImGui::GetID("view_year"), current_tm.tm_year + 1900);
            int* view_month = storage->GetIntRef(ImGui::GetID("view_month"), current_tm.tm_mon + 1);

            bool date_edited = RenderDatePickerUI(current_tm, *view_year, *view_month);
            ImGui::Separator();
            bool time_edited = RenderTimePickerUI(current_tm);

            if (date_edited || time_edited) {
                current_tm.tm_isdst = -1;
                std::time_t new_time = std::mktime(&current_tm);
                if (new_time >= 0) {
                    timestamp = std::chrono::sys_seconds(std::chrono::seconds(new_time));
                }
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }

    void DatePicker(const char* label, std::chrono::sys_seconds& date, const char* tooltip) {
        if (!label) return;
        ImGui::PushID(label);

        std::time_t t = date.time_since_epoch().count();
        if (t < 0) {
            t = 0;
            date = std::chrono::sys_seconds(std::chrono::seconds(0));
        }
        std::tm current_tm = GetSafeLocalTime(t);

        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d",
            current_tm.tm_year + 1900, current_tm.tm_mon + 1, current_tm.tm_mday);

        if (ImGui::Button(buffer, ImVec2(ImGui::CalcItemWidth(), 0))) {
            ImGui::OpenPopup("DatePopup");
        }
        if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);

        if (ImGui::BeginPopup("DatePopup")) {
            ImGuiStorage* storage = ImGui::GetStateStorage();
            int* view_year = storage->GetIntRef(ImGui::GetID("view_year"), current_tm.tm_year + 1900);
            int* view_month = storage->GetIntRef(ImGui::GetID("view_month"), current_tm.tm_mon + 1);

            if (RenderDatePickerUI(current_tm, *view_year, *view_month)) {
                current_tm.tm_isdst = -1;
                std::time_t new_time = std::mktime(&current_tm);
                if (new_time >= 0) {
                    date = std::chrono::sys_seconds(std::chrono::seconds(new_time));
                }
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }

    void TimePicker(const char* label, std::chrono::seconds& time, const char* tooltip) {
        if (!label) return;
        ImGui::PushID(label);

        auto total_seconds = time.count();
        if (total_seconds < 0) {
            total_seconds = 0;
            time = std::chrono::seconds(0);
        }

        int h = static_cast<int>(total_seconds / 3600);
        int m = static_cast<int>((total_seconds % 3600) / 60);
        int s = static_cast<int>(total_seconds % 60);

        std::tm current_tm = {};
        current_tm.tm_hour = h;
        current_tm.tm_min = m;
        current_tm.tm_sec = s;

        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",
            current_tm.tm_hour, current_tm.tm_min, current_tm.tm_sec);

        if (ImGui::Button(buffer, ImVec2(ImGui::CalcItemWidth(), 0))) {
            ImGui::OpenPopup("TimePopup");
        }
        if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);

        if (ImGui::BeginPopup("TimePopup")) {
            if (RenderTimePickerUI(current_tm)) {
                int new_total_seconds = (current_tm.tm_hour * 3600) +
                    (current_tm.tm_min * 60) +
                    current_tm.tm_sec;
                time = std::chrono::seconds(new_total_seconds);
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
    }

    void ServerAddressInput(const char* label, std::string& protocol, std::string& host, int& port, const std::vector<std::string>& protocols, float item_width, const char* tooltip) {
        if (!label) return;
        ImGui::PushID(label);

        bool is_hovered = false;
        float protocol_width = 120.0f;
        float port_width = 100.0f;

        ImGui::SetNextItemWidth(protocol_width);
        if (ImGui::BeginCombo("##Protocol", protocol.c_str())) {
            for (const auto& p : protocols) {
                bool is_selected = (protocol == p);
                if (ImGui::Selectable(p.c_str(), is_selected)) {
                    protocol = p;
                }
                if (is_selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
        is_hovered |= ImGui::IsItemHovered();

        ImGui::SameLine(0, 4.0f);

        ImGui::SetNextItemWidth(item_width - protocol_width - port_width - 8.0f);
        ImGui::InputText("##Host", &host);
        is_hovered |= ImGui::IsItemHovered();

        ImGui::SameLine(0, 4.0f);

        ImGui::SetNextItemWidth(port_width);
        ImGui::InputInt("##Port", &port, 0, 0);
        is_hovered |= ImGui::IsItemHovered();

        if (tooltip && is_hovered) {
            ImGui::SetTooltip("%s", tooltip);
        }

        ImGui::PopID();
    }

} // namespace floodsim::gui
