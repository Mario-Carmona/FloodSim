/**
 * \file MainControlTab.cpp
 * \brief Implementation of the Main Control tab.
 *
 * Provides the GUI layout for configuration management, simulation
 * start/stop controls, and the embedded terminal output for application logs.
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

namespace floodsim::gui {

    /**
    * \struct AppLog
    * \brief Manages and renders the internal simulation log console.
    */
    struct AppLog {
        struct LogMessage {
            std::string prefix;     // Contains: "[2026-05-02 19:41...] [Core    ] "
            std::string level_tag;  // Contains: "[info]"
            std::string payload;    // Contains: " Configuring ONNX Runtime..."
            ImVec4 color;
        };

        std::vector<LogMessage> items;
        std::mutex log_mutex;

        bool auto_scroll = true;       // Controls smart scrolling behavior
        bool scroll_to_bottom = false; // Manual trigger to force scrolling (single use)

        void Clear() {
            std::lock_guard<std::mutex> lock(log_mutex);
            items.clear();
        }

        void AddLog(int spdlog_level, const std::string& raw_msg) {
            try {
                // 1. spdlog always appends a '\n' to the formatted message.
                // We remove it for ImGui to prevent extra blank lines.
                std::string msg = raw_msg;
                if (!msg.empty() && msg.back() == '\n') {
                    msg.pop_back();
                }

                // 2. Map colors based on the spdlog level
                ImVec4 color;
                switch (spdlog_level) {
                    case 1: color = ImVec4(0.2f, 0.6f, 1.0f, 1.0f); break; // Debug (Blue)
                    case 2: color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); break; // Info (Green)
                    case 3: color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f); break; // Warn (Yellow)
                    case 4: color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); break; // Error (Red)
                    case 5: color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f); break; // Critical (Intense Red)
                    default: color = ImVec4(0.9f, 0.9f, 0.9f, 1.0f); break;
                }

                // 3. PATTERN PARSING: Find the third pair of brackets
                size_t pos1 = msg.find('[');
                size_t pos2 = msg.find('[', pos1 + 1);
                size_t pos3 = msg.find('[', pos2 + 1);
                size_t pos3_close = msg.find(']', pos3 + 1);

                std::string prefix, tag, payload;

                if (pos3 != std::string::npos && pos3_close != std::string::npos) {
                    // Successfully split
                    prefix = msg.substr(0, pos3);                  // From start up to the 3rd '['
                    tag = msg.substr(pos3, pos3_close - pos3 + 1); // Exactly "[info]" or "[debug]"
                    payload = msg.substr(pos3_close + 1);          // Everything else
                }
                else {
                    // Security fallback if the log doesn't follow the exact pattern
                    prefix = "";
                    tag = "";
                    payload = msg;
                }

                std::lock_guard<std::mutex> lock(log_mutex);
                items.push_back({ prefix, tag, payload, color });

            }
            catch (const std::exception& e) {
                std::cerr << "[Error] Exception caught while adding log: " << e.what() << "\n";
            }
        }

        void Draw(const char* title) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Terminal Output");
            ImGui::Spacing();

            if (ImGui::Checkbox("Auto-scroll", &auto_scroll)) {
                // If newly activated, force scroll to bottom
                if (auto_scroll) {
                    scroll_to_bottom = true;
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("Clear Log")) {
                Clear();
            }

            ImGui::BeginChild(title, ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

            {
                std::lock_guard<std::mutex> lock(log_mutex);
                for (const auto& item : items) {
                    if (!item.prefix.empty()) {
                        // 1. Draw prefix (Date + Thread) with default disabled color
                        ImGui::TextDisabled("%s", item.prefix.c_str());

                        // 2. Instruct ImGui to draw the next element on the same line
                        ImGui::SameLine(0, 0);

                        // 3. Draw the level tag with its specific color
                        ImGui::PushStyleColor(ImGuiCol_Text, item.color);
                        ImGui::TextUnformatted(item.level_tag.c_str());
                        ImGui::PopStyleColor();

                        ImGui::SameLine(0, 0);

                        // 4. Draw the payload text with normal interface color
                        ImGui::TextUnformatted(item.payload.c_str());
                    }
                    else {
                        // Fallback: If not separated, paint everything based on level color
                        ImGui::PushStyleColor(ImGuiCol_Text, item.color);
                        ImGui::TextUnformatted(item.payload.c_str());
                        ImGui::PopStyleColor();
                    }
                }
            }

            // --- SMART SCROLLING LOGIC ---
            // If manual scroll is triggered OR (auto-scroll is active AND we are at the bottom)
            if (scroll_to_bottom || (auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
                ImGui::SetScrollHereY(1.0f);
            }

            // Reset manual trigger to avoid locking the mouse on the next frame
            scroll_to_bottom = false;

            ImGui::EndChild();
        }
    };

    // Static variable to persist logs during execution
    static AppLog gui_console;

    void RenderMainControlTab(
        const danasim::Config& config,
        std::atomic<bool>& is_simulation_running,
        std::shared_ptr<danasim::Application>& current_simulation,
        std::jthread& simulation_thread) {

        try {
            ImGui::Spacing();

            // --- SECTION 1: CONFIGURATION MANAGEMENT ---
            ImGui::SeparatorText("Configuration Management");
            ImGui::Spacing();

            if (ImGui::Button("Save Configuration As...")) {
                auto destination = pfd::save_file("Save Scenario Config", "", { "YAML Files", "*.yaml" }).result();
                if (!destination.empty()) {
                    // Assuming ConfigLoader API remained the same externally
                    danasim::ConfigLoader::saveToFile(destination, config);
                }
            }

            ImGui::Spacing();
            ImGui::Spacing();

            // --- SECTION 2: CONTROL BUTTONS ---
            ImGui::SeparatorText("Simulation Control");
            ImGui::Spacing();

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

            // 1. Capture current state at the start of the frame
            bool is_running = is_simulation_running.load(std::memory_order_relaxed);

            // --- START BUTTON ---
            if (is_running) { ImGui::BeginDisabled(); }

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
            if (ImGui::Button("START SIMULATION", ImVec2(180, 45))) {
                gui_console.Clear();

                // Change state (local variable 'is_running' remains false for this frame)
                is_simulation_running.store(true, std::memory_order_relaxed);

                // 1. Prepare Callback
                auto log_to_gui = [](int level, const std::string& msg) {
                    gui_console.AddLog(level, msg);
                };

                // 2. Instantiate the engine
                current_simulation = std::make_shared<danasim::Application>(config, log_to_gui);

                // 3. Launch secondary thread
                simulation_thread = std::jthread([&current_simulation, &is_simulation_running]() {
                    // This block runs independently without affecting the GUI
                    if (current_simulation) {
                        current_simulation->run(); // Starts the heavy cycle
                    }

                    // When run() ends, notify completion
                    is_simulation_running.store(false, std::memory_order_relaxed);
                });
            }
            ImGui::PopStyleColor();

            if (is_running) { ImGui::EndDisabled(); }

            ImGui::SameLine();

            // --- STOP BUTTON ---
            // Logic inverted: if it is NOT running, disable it
            if (!is_running) { ImGui::BeginDisabled(); }

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("STOP", ImVec2(120, 45))) {
                if (current_simulation) {
                    current_simulation->stop();
                }
            }
            ImGui::PopStyleColor();

            if (!is_running) { ImGui::EndDisabled(); }

            ImGui::PopStyleVar();

            ImGui::Spacing();
            ImGui::Spacing();

            // --- SECTION 3: OUTPUT CONSOLE ---
            // Uses the remaining available space for the terminal
            gui_console.Draw("ConsoleWindow");

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
