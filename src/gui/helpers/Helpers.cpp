
#include "gui/helpers/Helpers.hpp"

#include <sstream>
#include <iomanip>

namespace FloodSim::Gui {

    // Helper para campos de texto estándar
    void TextInput(const char* label, std::string& value, const char* tooltip) {
        ImGui::PushID(label); // Evita colisiones de IDs

        ImGui::InputText("##input", &value);

        if (tooltip && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }

        ImGui::PopID();
    }

    void FloatInput(const char* label, float& value, const char* tooltip) {
        ImGui::PushID(label);

        ImGui::InputFloat("##input", &value, 0.0f, 0.0f, "%.7g");

        if (tooltip && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }

        ImGui::PopID();
    }

    void DoubleInput(const char* label, double& value, const char* tooltip) {
        ImGui::PushID(label);

        ImGui::InputDouble("##input", &value, 0.0, 0.0, "%.8g");

        if (tooltip && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }

        ImGui::PopID();
    }

    void Int64Input(const char* label, int64_t& value, const char* tooltip) {
        ImGui::PushID(label);

        int64_t step = 1;
        int64_t step_fast = 100; // Lo que suma si mantienes pulsado o usas el modo rápido

        // Usamos InputScalar indicando explícitamente que es un entero de 64 bits (S64)
        ImGui::InputScalar("##input", ImGuiDataType_S64, &value, &step, &step_fast);

        if (tooltip && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }

        ImGui::PopID();
    }

    // Helper para booleanos (Checkbox)
    void Checkbox(const char* label, bool& value, const char* tooltip) {
        ImGui::PushID(label);

        ImGui::Checkbox("##checkbox", &value);

        if (tooltip && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }

        ImGui::PopID();
    }

    // Helper para selección de carpetas
    void FolderInput(const char* label, std::filesystem::path& value, const char* tooltip) {
        ImGui::PushID(label);
        
        std::string tempStr = value.string();

        float browseButtonWidth = 80.0f;

        if (ImGui::Button("Browse", ImVec2(browseButtonWidth, 0))) {
            auto folder = pfd::select_folder("Select Directory", tempStr).result();
            if (!folder.empty()) {
                value = folder;
            }
        }

        if (tooltip && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }

        ImGui::SameLine();

        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputText("##input", &tempStr, ImGuiInputTextFlags_ReadOnly);

        ImGui::PopID();
    }

    void ColorInput(const char* label, std::string& hexColor, const char* tooltip, ImGuiColorEditFlags extraFlags) {
        ImGui::PushID(label);

        unsigned int r = 255, g = 255, b = 255, a = 255;
        if (hexColor.length() >= 6) std::sscanf(hexColor.c_str(), "%02x%02x%02x", &r, &g, &b);
        if (hexColor.length() >= 8) std::sscanf(hexColor.c_str() + 6, "%02x", &a);
        float col[4] = { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };

        // Flags por defecto combinados con los que se pasen por parámetro
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_AlphaPreview | extraFlags;

        if (ImGui::ColorEdit4(label, col, flags)) {
            char buffer[10];
            std::snprintf(buffer, sizeof(buffer), "%02X%02X%02X%02X",
                (int)(col[0] * 255.0f), (int)(col[1] * 255.0f),
                (int)(col[2] * 255.0f), (int)(col[3] * 255.0f));
            hexColor = buffer;
        }

        if (tooltip && ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", tooltip);
        }

        ImGui::PopID();
    }

    // Funciones internas de ayuda para el calendario
    namespace {
        int getDaysInMonth(int year, int month) {
            static const int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
            if (month == 2) {
                bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
                return leap ? 29 : 28;
            }
            return days[month - 1];
        }

        const char* getMonthName(int month) {
            static const char* names[] = { "January", "February", "March", "April", "May", "June",
                                           "July", "August", "September", "October", "November", "December" };
            return names[month - 1];
        }

        // Función DRY para obtener la fecha de forma segura
        std::tm getSafeLocalTime(std::time_t t) {
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

        // --- SUB-MÉTODO 1: CALENDARIO ---
        bool RenderDatePickerUI(std::tm& current_tm, int& view_year, int& view_month) {
            bool changed = false;

            // Cabecera de navegación
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 4));
            if (ImGui::Button("<", ImVec2(24, 0))) {
                view_month -= 1;
                if (view_month < 1) { view_month = 12; view_year -= 1; }
            }
            ImGui::SameLine();

            char month_str[64];
            std::snprintf(month_str, sizeof(month_str), "%s %d", getMonthName(view_month), view_year);
            float text_width = ImGui::CalcTextSize(month_str).x;
            float center_pos = (ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x - text_width) * 0.5f;
            ImGui::SetCursorPosX(center_pos);
            ImGui::Text("%s", month_str);

            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - 24);
            if (ImGui::Button(">", ImVec2(24, 0))) {
                view_month += 1;
                if (view_month > 12) { view_month = 1; view_year += 1; }
            }
            ImGui::PopStyleVar();
            ImGui::Separator();

            // Rejilla de días
            if (ImGui::BeginTable("DaysTable", 7, ImGuiTableFlags_SizingStretchSame)) {
                const char* days[] = { "Mo", "Tu", "We", "Th", "Fr", "Sa", "Su" };
                for (int i = 0; i < 7; ++i) ImGui::TableSetupColumn(days[i]);
                ImGui::TableHeadersRow();

                std::tm time_in = { 0, 0, 0, 1, view_month - 1, view_year - 1900 };
                std::mktime(&time_in);
                int start_day_offset = (time_in.tm_wday + 6) % 7;
                int days_in_month = getDaysInMonth(view_year, view_month);

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

                    bool is_selected_date = (day == current_tm.tm_mday && view_month == current_tm.tm_mon + 1 && view_year == current_tm.tm_year + 1900);
                    if (is_selected_date) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

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

        // --- SUB-MÉTODO 2: HORA ---
        bool RenderTimePickerUI(std::tm& current_tm) {
            bool changed = false;
            ImGui::TextDisabled("Time (HH:MM:SS)");

            int h = current_tm.tm_hour;
            int m = current_tm.tm_min;
            int s = current_tm.tm_sec;

            auto TimeComboBox = [](const char* id, int& val, int max_val) -> bool {
                bool local_changed = false;
                char preview[8];
                std::snprintf(preview, sizeof(preview), "%02d", val);

                ImGui::PushItemWidth(50.0f);
                if (ImGui::BeginCombo(id, preview, ImGuiComboFlags_HeightLargest)) {
                    for (int i = 0; i <= max_val; ++i) {
                        char item[8];
                        std::snprintf(item, sizeof(item), "%02d", i);
                        bool is_selected = (val == i);
                        if (ImGui::Selectable(item, is_selected)) { val = i; local_changed = true; }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();
                return local_changed;
                };

            if (TimeComboBox("##h", h, 23)) changed = true;
            ImGui::SameLine(0, 4.0f); ImGui::Text(":"); ImGui::SameLine(0, 4.0f);
            if (TimeComboBox("##m", m, 59)) changed = true;
            ImGui::SameLine(0, 4.0f); ImGui::Text(":"); ImGui::SameLine(0, 4.0f);
            if (TimeComboBox("##s", s, 59)) changed = true;

            if (changed) {
                current_tm.tm_hour = h;
                current_tm.tm_min = m;
                current_tm.tm_sec = s;
            }

            return changed;
        }
    }

    

    void TimestampPicker(const char* label, std::chrono::sys_seconds& timestamp, const char* tooltip) {
        ImGui::PushID(label);

        std::time_t t = timestamp.time_since_epoch().count();
        if (t < 0) { t = 0; timestamp = std::chrono::sys_seconds(std::chrono::seconds(0)); }
        std::tm current_tm = getSafeLocalTime(t);

        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
            current_tm.tm_year + 1900, current_tm.tm_mon + 1, current_tm.tm_mday,
            current_tm.tm_hour, current_tm.tm_min, current_tm.tm_sec);

        if (ImGui::Button(buffer, ImVec2(-FLT_MIN, 0))) ImGui::OpenPopup("TimestampPopup");
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
                if (new_time >= 0) { timestamp = std::chrono::sys_seconds(std::chrono::seconds(new_time)); }
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    // --- 2. WIDGET SÓLO FECHA ---
    void DatePicker(const char* label, std::chrono::sys_seconds& date, const char* tooltip) {
        ImGui::PushID(label);

        std::time_t t = date.time_since_epoch().count();
        if (t < 0) { t = 0; date = std::chrono::sys_seconds(std::chrono::seconds(0)); }
        std::tm current_tm = getSafeLocalTime(t);

        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d",
            current_tm.tm_year + 1900, current_tm.tm_mon + 1, current_tm.tm_mday);

        if (ImGui::Button(buffer, ImVec2(-FLT_MIN, 0))) ImGui::OpenPopup("DatePopup");
        if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);

        if (ImGui::BeginPopup("DatePopup")) {
            ImGuiStorage* storage = ImGui::GetStateStorage();
            int* view_year = storage->GetIntRef(ImGui::GetID("view_year"), current_tm.tm_year + 1900);
            int* view_month = storage->GetIntRef(ImGui::GetID("view_month"), current_tm.tm_mon + 1);

            if (RenderDatePickerUI(current_tm, *view_year, *view_month)) {
                current_tm.tm_isdst = -1;
                std::time_t new_time = std::mktime(&current_tm);
                if (new_time >= 0) { date = std::chrono::sys_seconds(std::chrono::seconds(new_time)); }
                // Opcional: Descomentar la siguiente línea si quieres que al clicar un día el popup se cierre
                // ImGui::CloseCurrentPopup(); 
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    // --- 3. WIDGET SÓLO HORA ---
    void TimePicker(const char* label, std::chrono::seconds& time, const char* tooltip) {
        ImGui::PushID(label);

        // 1. Obtenemos los segundos totales directamente (es una duración, no una fecha)
        auto total_seconds = time.count();
        if (total_seconds < 0) {
            total_seconds = 0;
            time = std::chrono::seconds(0);
        }

        // 2. Calculamos las horas, minutos y segundos con matemáticas simples
        int h = total_seconds / 3600;
        int m = (total_seconds % 3600) / 60;
        int s = total_seconds % 60;

        // 3. Creamos un std::tm "dummy" solo para que la UI pueda leerlo y modificarlo
        std::tm current_tm = {};
        current_tm.tm_hour = h;
        current_tm.tm_min = m;
        current_tm.tm_sec = s;

        // 4. Formateamos el texto del botón
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d",
            current_tm.tm_hour, current_tm.tm_min, current_tm.tm_sec);

        if (ImGui::Button(buffer, ImVec2(-FLT_MIN, 0))) ImGui::OpenPopup("TimePopup");
        if (tooltip && ImGui::IsItemHovered()) ImGui::SetTooltip("%s", tooltip);

        // 5. Renderizamos el popup
        if (ImGui::BeginPopup("TimePopup")) {
            // Tu función modificará current_tm.tm_hour, tm_min y tm_sec internamente
            if (RenderTimePickerUI(current_tm)) {

                // 6. Reconstruimos la duración total en segundos y la guardamos
                int new_total_seconds = (current_tm.tm_hour * 3600) +
                    (current_tm.tm_min * 60) +
                    current_tm.tm_sec;

                time = std::chrono::seconds(new_total_seconds);
            }
            ImGui::EndPopup();
        }

        ImGui::PopID();
    }

    void ServerAddressInput(const char* label, std::string& protocol, std::string& host, int& port, const std::vector<std::string>& protocols, const char* tooltip) {
        ImGui::PushID(label);

        bool is_hovered = false;

        // 1. Protocolo (ComboBox de ancho fijo, ej: 85px)
        ImGui::SetNextItemWidth(85.0f);
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

        // Calculamos el espacio restante. Reservamos unos 60px para el puerto.
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float portWidth = 60.0f;

        // 2. Host (Se estira para ocupar el espacio intermedio)
        ImGui::SetNextItemWidth(availableWidth - portWidth - 4.0f);
        ImGui::InputText("##Host", &host);
        is_hovered |= ImGui::IsItemHovered();

        ImGui::SameLine(0, 4.0f);

        // 3. Puerto (Ocupa hasta el final. Los '0, 0' ocultan los botones de +/-)
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputInt("##Port", &port, 0, 0);
        is_hovered |= ImGui::IsItemHovered();

        // 4. Tooltip (Se muestra si el ratón está encima de CUALQUIERA de los 3 controles)
        if (tooltip && is_hovered) {
            ImGui::SetTooltip("%s", tooltip);
        }

        ImGui::PopID();
    }
}
