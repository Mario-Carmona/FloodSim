/**
 * @file mainGUI.cpp
 * @brief Entry point and main GUI loop for the Flood Simulator application.
 *
 * This file coordinates the initialization of the graphics subsystem (GLFW,
 * OpenGL), the setup of the Dear ImGui environment, and handles the multithreaded
 * runtime management to run simulations asynchronously. It includes safe window
 * closing interceptions to prevent data corruption during active tasks.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include <atomic>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <portable-file-dialogs.h>

#include "app/Application.hpp"
#include "app/config/Config.hpp"
#include "app/config/ConfigLoader.hpp"
#include "gui/tabs/Tabs.hpp"

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <Windows.h>
#endif

namespace {

/**
 * @brief Thread-safe atomic flag indicating if the simulation execution is active.
 */
std::atomic<bool> g_is_simulation_running{ false };

/**
 * @brief Shared pointer referencing the active instance of the simulation core application.
 */
std::shared_ptr<floodsim::app::Application> g_current_simulation = nullptr;

/**
 * @brief Cooperative background execution thread handling simulation frame updates.
 */
std::jthread g_simulation_thread;

/**
 * @brief Active structural parameters and options configured for the simulation context.
 */
floodsim::app::config::Config g_current_config;

/**
 * @brief State flag to control the rendering lifecycle of the unsaved simulation exit modal.
 */
bool g_show_exit_confirmation_modal = false;

/**
 * @brief Flag used to force window closing bypass after explicit user confirmation in the GUI modal.
 */
bool g_force_close_application = false;

/**
 * @brief Flag used to trigger the display of the "About" popup window.
 */
bool g_trigger_about_popup = false;

/**
 * @brief Native error callback routed directly from the underlying GLFW subsystem.
 * @param error_code The descriptive identifier code representing the internal GLFW error.
 * @param description A null-terminated string detailing the human-readable driver issue.
 */
void GlfwErrorCallback(int error_code, const char* description) {
    std::cerr << "[GLFW Error] (" << error_code << "): " << description << "\n";
}

/**
 * @brief Intercepts the native window close events from the operating system window manager.
 * * If a background simulation is actively running, it aborts the automatic window
 * destruction pipeline and triggers the internal Dear ImGui safe closure modal instead.
 * * @param window Pointer to the native GLFW window object instance.
 */
void WindowCloseCallback(GLFWwindow* window) {
    if (g_force_close_application) {
        return;
    }

    if (g_is_simulation_running.load()) {
        // Abort the native closure process and delegate handling to the ImGui render loop.
        glfwSetWindowShouldClose(window, GLFW_FALSE);
        g_show_exit_confirmation_modal = true;
    }
}

}  // namespace

namespace floodsim::gui {

/**
 * @enum ViewState
 * @brief Represents the high-level states of the application's user interface.
 */
enum class ViewState {
    kHome,          ///< The initial welcome screen.
    kNewScenario,   ///< The view for creating a new scenario from scratch.
    kLoadScenario   ///< The view for working with a loaded scenario.
};

/**
 * @brief Renders the "About" popup window.
 */
void RenderAboutWindow() {
    ImGui::SetNextWindowSize(ImVec2(500, 450), ImGuiCond_FirstUseEver);

    bool is_open = true;

    if (ImGui::BeginPopupModal(("About " + std::string(FLOODSIM_PROGRAM_NAME)).c_str(), &is_open, ImGuiWindowFlags_NoCollapse)) {
        ImGui::TextUnformatted((std::string(FLOODSIM_PROGRAM_NAME) + " - Flood Simulation Engine").c_str());
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Text("Version: %s", FLOODSIM_PROGRAM_VERSION);
        ImGui::Text("Copyright (c) %s %s", FLOODSIM_COPYRIGHT_YEAR, FLOODSIM_AUTHOR);
        ImGui::Spacing();

        ImGui::TextWrapped(
            "FloodSim is a hydrological simulation engine developed as a Master's Thesis. "
            "It leverages geospatial and meteorological data to model real-time flood behavior "
            "and spatiotemporal dynamic risks."
        );
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::CollapsingHeader("Software License (MIT)")) {
            ImGui::BeginChild("LicenseText", ImVec2(0, 180), true);

            ImGui::TextWrapped(
                "MIT License\n\n"
                "Copyright(c) 2026 Mario Carmona Segovia\n\n"
                "Permission is hereby granted, free of charge, to any person obtaining a copy "
                "of this software and associated documentation files(the \"Software\"), to deal "
                "in the Software without restriction, including without limitation the rights "
                "to use, copy, modify, merge, publish, distribute, sublicense, and /or sell "
                "copies of the Software, and to permit persons to whom the Software is "
                "furnished to do so, subject to the following conditions :\n\n"
                "The above copyright notice and this permission notice shall be included in all "
                "copies or substantial portions of the Software.\n\n"
                "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR "
                "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, "
                "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE "
                "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER "
                "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, "
                "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE "
                "SOFTWARE."
            );

            ImGui::EndChild();
        } 

        ImGui::EndPopup();
    }
}

/**
 * @brief Renders the main configuration tabs based on the current configuration.
 *
 * Disables certain tabs while the simulation is actively running to prevent
 * concurrent modification of critical parameters.
 *
 * @param config Reference to the active scenario configuration.
 */
void RenderTabs(app::config::Config& config) {
    ImGui::Spacing();

    bool is_running = g_is_simulation_running.load(std::memory_order_relaxed);

    if (ImGui::BeginTabBar("ConfigurationTabs")) {
        if (ImGui::BeginTabItem("Main Control")) {
            RenderMainControlTab(
                config,
                g_is_simulation_running,
                g_current_simulation,
                g_simulation_thread
            );
            ImGui::EndTabItem();
        }

        if (is_running) {
            ImGui::BeginDisabled();
        }

        if (ImGui::BeginTabItem("Scenario Info")) {
            RenderScenarioTab(config.scenario);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Simulation")) {
            RenderSimulationTab(config.simulation);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Logging")) {
            RenderLoggingTab(config.logging);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("State Updater")) {
            RenderStateUpdaterTab(config.state_updater);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Input")) {
            RenderInputTab(config.input, config.state_updater.updater);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Output")) {
            RenderOutputTab(config.output);
            ImGui::EndTabItem();
        }

        if (is_running) {
            ImGui::EndDisabled();
        }

        ImGui::EndTabBar();
    }
}

}  // namespace floodsim::gui

/**
 * @brief Subsystem bootstrap and master graphical interface event loop.
 *
 * @param argc The number of command-line arguments provided to the process.
 * @param argv An array of null-terminated strings representing the execution arguments.
 * @return int Returns EXIT_SUCCESS upon a graceful termination, or EXIT_FAILURE on crash.
 */
int main(int argc, char** argv) {
    glfwSetErrorCallback(GlfwErrorCallback);

    try {
        // ------------------------------------------------------------------------
        // Graphic Subsystem Initialization and Verification
        // ------------------------------------------------------------------------
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize the native GLFW window library.");
        }

        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

        GLFWwindow* window = glfwCreateWindow(1280, 720, (std::string(FLOODSIM_PROGRAM_NAME) + " v" + std::string(FLOODSIM_PROGRAM_VERSION)).c_str(), nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to allocate or create the host GLFW native window context.");
        }

        #ifdef _WIN32
		// Get the HWND (window handle) from GLFW
        HWND hwnd = glfwGetWin32Window(window);

		// Get the instance handle of the current process
        HINSTANCE hinstance = GetModuleHandle(NULL);

		// Load the icon from the .rc resource file using its name/ID
        HICON hIcon = LoadIcon(hinstance, "IDI_ICON1");
        if (hIcon) {
			// Set the big icon (Taskbar and Alt+Tab)
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			// Set the small icon (Window corner)
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        }
        #endif

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync

        // Register our closure interceptor callback.
        glfwSetWindowCloseCallback(window, WindowCloseCallback);

        // ------------------------------------------------------------------------
        // Dear ImGui Engine Bootstrapping
        // ------------------------------------------------------------------------
        IMGUI_CHECKVERSION();
        if (!ImGui::CreateContext()) {
            glfwDestroyWindow(window);
            glfwTerminate();
            throw std::runtime_error("Failed to allocate structural memory for Dear ImGui engine context.");
        }

        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.FontGlobalScale = 1.3f;

        ImGui::StyleColorsDark();

        if (!ImGui_ImplGlfw_InitForOpenGL(window, true) || !ImGui_ImplOpenGL3_Init("#version 330")) {
            ImGui::DestroyContext();
            glfwDestroyWindow(window);
            glfwTerminate();
            throw std::runtime_error("Failed to link system GLFW platform and OpenGL3 interfaces to ImGui context.");
        }

        floodsim::gui::ViewState current_state = floodsim::gui::ViewState::kHome;
        bool is_dark_theme = true;

        // ------------------------------------------------------------------------
        // Master Graphical Frame Execution Loop
        // ------------------------------------------------------------------------
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            try {
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                // Establish the main full-screen docking workspace canvas area.
                const ImGuiViewport* viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->WorkPos);
                ImGui::SetNextWindowSize(viewport->WorkSize);

                constexpr ImGuiWindowFlags kWindowFlags =
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar;

                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                ImGui::Begin("FloodSim Master Workspace", nullptr, kWindowFlags);
                ImGui::PopStyleVar();

                if (ImGui::BeginMenuBar()) {
                    if (ImGui::BeginMenu("Scenario")) {
                        if (ImGui::MenuItem("New Scenario (Blank)")) {
                            current_state = floodsim::gui::ViewState::kNewScenario;
                            g_current_config = floodsim::app::config::Config();
                        }

                        if (ImGui::MenuItem("Load Existing Configuration...")) {
                            auto selection = pfd::open_file(
                                "Select Configuration File",
                                ".",
                                { "YAML Files", "*.yaml" }
                            ).result();

                            if (!selection.empty()) {
                                std::string config_file_path = selection[0];
                                try {
                                    g_current_config = floodsim::app::config::ConfigLoader::Load(config_file_path);
                                    current_state = floodsim::gui::ViewState::kLoadScenario;
                                }
                                catch (const std::exception& e) {
                                    std::cerr << "[Error] Failed to load configuration: " << e.what() << "\n";
                                }
                            }
                        }

                        ImGui::Separator();

                        if (ImGui::MenuItem("Exit", "Alt+F4")) {
                            glfwSetWindowShouldClose(window, true);
                        }

                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("View")) {
                        if (ImGui::MenuItem(is_dark_theme ? "Switch to Light Theme" : "Switch to Dark Theme")) {
                            is_dark_theme = !is_dark_theme;
                            if (is_dark_theme) {
                                ImGui::StyleColorsDark();
                            }
                            else {
                                ImGui::StyleColorsLight();
                            }
                        }
                        ImGui::EndMenu();
                    }

                    if (ImGui::BeginMenu("Help")) {
                        if (ImGui::MenuItem(("About " + std::string(FLOODSIM_PROGRAM_NAME) + "...").c_str())) {
                            g_trigger_about_popup = true;
                        }
                        ImGui::EndMenu();
                    }

                    ImGui::EndMenuBar();
                }

                if (g_trigger_about_popup) {
                    ImGui::OpenPopup("About FloodSim");
                    g_trigger_about_popup = false;
                }

                floodsim::gui::RenderAboutWindow();

                // Route state render graphs depending on the underlying active view-state.
                switch (current_state) {
                case floodsim::gui::ViewState::kHome: {
                    const char* welcome_text = "Select an option from the 'Scenario' menu to begin.";
                    ImGui::SetWindowFontScale(2.0f);

                    ImVec2 text_size = ImGui::CalcTextSize(welcome_text);
                    ImVec2 window_size = ImGui::GetWindowSize();

                    ImGui::SetCursorPos(ImVec2(
                        (window_size.x - text_size.x) * 0.5f,
                        (window_size.y - text_size.y) * 0.5f
                    ));

                    ImGui::TextDisabled("%s", welcome_text);
                    ImGui::SetWindowFontScale(1.0f);
                    break;
                }
                case floodsim::gui::ViewState::kNewScenario:
                case floodsim::gui::ViewState::kLoadScenario: {
                    floodsim::gui::RenderTabs(g_current_config);
                    break;
                }
                }

                // --------------------------------------------------------------------
                // Intercepted Exit Confirmation Modal Rendering
                // --------------------------------------------------------------------
                if (g_show_exit_confirmation_modal) {
                    ImGui::OpenPopup("Confirm Exit");
                }

                // Set layout constraints to ensure the modal window appears perfectly centered.
                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

                if (ImGui::BeginPopupModal("Confirm Exit", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::Text("Warning: A simulation is currently running in the background.");
                    ImGui::Text("Closing the application now will forcefully stop the calculation threads.");
                    ImGui::Text("Do you want to stop the simulation and exit anyway?");
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    if (ImGui::Button("Yes, Exit and Stop", ImVec2(200, 35))) {
                        ImGui::CloseCurrentPopup();
                        g_show_exit_confirmation_modal = false;
                        g_force_close_application = true;

                        // Forcefully break the frame loop via the native subsystem on next cycle evaluation.
                        glfwSetWindowShouldClose(window, GLFW_TRUE);
                    }

                    ImGui::SameLine();

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                    if (ImGui::Button("Cancel", ImVec2(120, 35))) {
                        ImGui::CloseCurrentPopup();
                        g_show_exit_confirmation_modal = false;
                    }
                    ImGui::PopStyleColor(2);

                    ImGui::EndPopup();
                }

                ImGui::End(); // Close workspace window block
                ImGui::Render();

                int display_width = 0;
                int display_height = 0;
                glfwGetFramebufferSize(window, &display_width, &display_height);
                glViewport(0, 0, display_width, display_height);
                glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                glfwSwapBuffers(window);

            } catch (const std::exception& inner_e) {
                std::cerr << "[Warning] Exception intercepted during layout frame rendering: " << inner_e.what() << "\n";
                // Attempting to finish the frame gracefully to prevent ImGui internal assertion failures
                ImGui::EndFrame();
            }
        }

        // ------------------------------------------------------------------------
        // Safe Thread Halting and Runtime Interruption
        // ------------------------------------------------------------------------
        if (g_is_simulation_running.load() && g_current_simulation) {
            std::cout << "[GUI] Shutdown signal initiated. Halting active simulation threads cleanly...\n";
            try {
                g_current_simulation->Stop();
                // Implicit cooperative cancellation will safely block via std::jthread join constraints.
            }
            catch (const std::exception& stop_e) {
                std::cerr << "[Warning] Exception caught during shutdown stop execution: " << stop_e.what() << "\n";
            }
        }

        // ------------------------------------------------------------------------
        // Resource Deallocation and Subsystem Teardown
        // ------------------------------------------------------------------------
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();

        return EXIT_SUCCESS;

    } catch (const std::exception& e) {
        std::cerr << "[Fatal Error] High-level catastrophic crash caught: " << e.what() << "\n";
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "[Fatal Error] Unknown critical collapse occurred within the hardware stack.\n";
        return EXIT_FAILURE;
    }
}
