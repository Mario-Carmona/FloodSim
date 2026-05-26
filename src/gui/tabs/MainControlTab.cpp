/**
 * @file MainControlTab.cpp
 * @brief Implementation of the Main Control tab.
 *
 * Provides the GUI layout for configuration management, simulation
 * start/stop controls, and the embedded terminal output for application logs.
 * 
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "gui/tabs/Tabs.hpp"

#include <exception>
#include <iostream>
#include <mutex>
#include <string>
#include <vector>

#include <imgui.h>
#include <portable-file-dialogs.h>

#include "app/config/ConfigLoader.hpp"
#include "logging/LoggerLevel.hpp"

namespace floodsim::gui {

namespace {

/**
 * @struct LogMessage
 * @brief Represents a single parsed log entry in the GUI console.
 */
struct LogMessage {
    std::string prefix;     ///< The timestamp and thread info (e.g., "[2026-05-02 19:41:00] [Main]")
    std::string level_tag;  ///< The log level tag (e.g., "[info]")
    std::string payload;    ///< The actual message content
    ImVec4 color;           ///< Color assigned based on the log severity
};

/**
 * @class AppLog
 * @brief Thread-safe manager for the embedded GUI log console.
 */
class AppLog {
public:
    /**
     * @brief Clears all current log messages.
     */
    void Clear() {
        std::lock_guard<std::mutex> lock(log_mutex_);
        items_.clear();
    }

    /**
     * @brief Appends a new log message to the console.
     * @param level The severity level of the log (mapped to spdlog/LoggerLevel).
     * @param raw_msg The complete formatted string from the logging backend.
     */
    void AddLog(int level, const std::string& raw_msg) {
        std::lock_guard<std::mutex> lock(log_mutex_);

        LogMessage msg;

        // Simple mapping from spdlog int levels to ImGui colors
        // Assuming: 0=Trace, 1=Debug, 2=Info, 3=Warn, 4=Err, 5=Critical
        switch (level) {
        case 0: msg.color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f); break; // Trace (Gray)
        case 1: msg.color = ImVec4(0.2f, 0.8f, 0.8f, 1.0f); break; // Debug (Cyan)
        case 2: msg.color = ImVec4(0.4f, 0.9f, 0.4f, 1.0f); break; // Info (Green)
        case 3: msg.color = ImVec4(0.9f, 0.9f, 0.2f, 1.0f); break; // Warn (Yellow)
        case 4: msg.color = ImVec4(0.9f, 0.3f, 0.3f, 1.0f); break; // Error (Red)
        case 5: msg.color = ImVec4(1.0f, 0.0f, 1.0f, 1.0f); break; // Critical (Magenta)
        default: msg.color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f); break; // Default (White)
        }

        // Extremely basic parse to separate the tag for coloring purposes.
        // Expecting format: "[Date Time] [Thread] [level] Message"
        size_t last_bracket = raw_msg.find_last_of(']');
        if (last_bracket != std::string::npos && raw_msg.length() > last_bracket + 2) {
            size_t second_last_bracket = raw_msg.find_last_of('[', last_bracket);
            if (second_last_bracket != std::string::npos) {
                msg.prefix = raw_msg.substr(0, second_last_bracket);
                msg.level_tag = raw_msg.substr(second_last_bracket, last_bracket - second_last_bracket + 1);
                msg.payload = raw_msg.substr(last_bracket + 1);
            }
            else {
                msg.payload = raw_msg;
            }
        }
        else {
            msg.payload = raw_msg;
        }

        items_.push_back(msg);
        if (auto_scroll_) {
            scroll_to_bottom_ = true;
        }
    }

    /**
     * @brief Renders the log console widget.
     * @param title The ImGui ID for the child window.
     */
    void Draw(const char* title) {
        try {
            if (ImGui::Button("Clear Log")) {
                Clear();
            }
            ImGui::SameLine();
            bool current_auto_scroll = auto_scroll_;
            if (ImGui::Checkbox("Auto-scroll", &current_auto_scroll)) {
                auto_scroll_ = current_auto_scroll;
            }

            ImGui::Separator();

            // Draw the scrolling region
            ImGui::BeginChild(title, ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

            {
                std::lock_guard<std::mutex> lock(log_mutex_);
                for (const auto& item : items_) {
                    if (!item.prefix.empty()) {
                        ImGui::TextUnformatted(item.prefix.c_str());
                        ImGui::SameLine(0, 0);
                    }
                    if (!item.level_tag.empty()) {
                        ImGui::PushStyleColor(ImGuiCol_Text, item.color);
                        ImGui::TextUnformatted(item.level_tag.c_str());
                        ImGui::PopStyleColor();
                        ImGui::SameLine(0, 0);
                    }
                    ImGui::TextUnformatted(item.payload.c_str());
                }
            }

            if (scroll_to_bottom_ || (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
                ImGui::SetScrollHereY(1.0f);
                scroll_to_bottom_ = false;
            }

            ImGui::EndChild();
        }
        catch (const std::exception& e) {
            std::cerr << "[GUI Error] Exception in AppLog::Draw: " << e.what() << '\n';
        }
    }

private:
    std::vector<LogMessage> items_;
    std::mutex log_mutex_;
    bool auto_scroll_ = true;
    bool scroll_to_bottom_ = false;
};

// Global instance for the GUI logger
AppLog g_gui_console;

} // anonymous namespace

/**
 * @brief Global callback passed to the logging system to route messages to the GUI.
 * @param level The log severity level.
 * @param raw_msg The fully formatted log string.
 */
void GlobalGuiLogCallback(int level, const std::string& raw_msg) {
    g_gui_console.AddLog(level, raw_msg);
}

void RenderMainControlTab(Config& config, std::atomic<bool>& is_simulation_running, std::shared_ptr<Application>& current_simulation, std::jthread& simulation_thread) {
    try {
        bool is_running = is_simulation_running.load(std::memory_order_relaxed);

        // ==========================================
        // 1. CONFIGURATION MANAGEMENT
        // ==========================================
        ImGui::Spacing();
        ImGui::SeparatorText("Configuration Management");
        ImGui::Spacing();

        // Disable load/save if simulation is running to prevent data races on config
        if (is_running) ImGui::BeginDisabled();

        if (ImGui::Button("Load Configuration", ImVec2(200, 30))) {
            try {
                auto result = pfd::open_file("Select Configuration File", "", { "YAML Files", "*.yaml *.yml", "All Files", "*" }).result();
                if (!result.empty()) {
                    config = ConfigLoader::Load(result[0]);
                    g_gui_console.AddLog(2, "[GUI] Configuration loaded successfully from: " + result[0] + "\n");
                }
            }
            catch (const std::exception& e) {
                g_gui_console.AddLog(4, std::string("[GUI Error] Failed to load configuration: ") + e.what() + "\n");
            }
        }

        ImGui::SameLine();

        if (ImGui::Button("Save Configuration", ImVec2(200, 30))) {
            try {
                auto destination = pfd::save_file("Save Configuration As", "config.yaml", { "YAML Files", "*.yaml *.yml" }).result();
                if (!destination.empty()) {
                    ConfigLoader::SaveToFile(destination, config);
                    g_gui_console.AddLog(2, "[GUI] Configuration saved successfully to: " + destination + "\n");
                }
            }
            catch (const std::exception& e) {
                g_gui_console.AddLog(4, std::string("[GUI Error] Failed to save configuration: ") + e.what() + "\n");
            }
        }

        if (is_running) ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::Spacing();

        // ==========================================
        // 2. SIMULATION CONTROLS
        // ==========================================
        ImGui::SeparatorText("Simulation Control");
        ImGui::Spacing();

        // --- START BUTTON ---
        if (is_running) ImGui::BeginDisabled();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        if (ImGui::Button("START", ImVec2(120, 45))) {
            try {
                current_simulation = std::make_shared<Application>(config, GlobalGuiLogCallback);
                is_simulation_running.store(true);
                simulation_thread = std::jthread([&]() { current_simulation->Run(); is_simulation_running.store(false); });
                g_gui_console.AddLog(2, "[GUI] Simulation startup sequence initiated.\n");
            }
            catch (const std::exception& e) {
                g_gui_console.AddLog(5, std::string("[GUI Fatal Error] Could not start simulation: ") + e.what() + "\n");
            }
        }
        ImGui::PopStyleColor();

        if (is_running) ImGui::EndDisabled();

        ImGui::SameLine();

        // --- STOP BUTTON ---
        if (!is_running) ImGui::BeginDisabled();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("STOP", ImVec2(120, 45))) {
            try {
                if (current_simulation) {
                    current_simulation->Stop();
                    g_gui_console.AddLog(3, "[GUI] Stop signal sent to the simulation engine. Waiting for safe halt...\n");
                }
            }
            catch (const std::exception& e) {
                g_gui_console.AddLog(5, std::string("[GUI Fatal Error] Exception during simulation stop sequence: ") + e.what() + "\n");
            }
        }
        ImGui::PopStyleColor();

        if (!is_running) ImGui::EndDisabled();

        ImGui::Spacing();
        ImGui::Spacing();

        // ==========================================
        // 3. OUTPUT CONSOLE
        // ==========================================
        ImGui::SeparatorText("Application Log Terminal");
        g_gui_console.Draw("ConsoleWindow");

    }
    catch (const std::exception& e) {
        std::cerr << "[Error] Exception caught while rendering Main Control Tab: " << e.what() << "\n";
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "An error occurred rendering the Main Control Tab.");
    }
    catch (...) {
        std::cerr << "[Error] Unknown exception caught while rendering Main Control Tab.\n";
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "A critical unknown error occurred rendering the Main Control Tab.");
    }
}

}  // namespace floodsim::gui
