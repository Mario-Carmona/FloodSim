/**
 * \file mainGUI.cpp
 * \brief Entry point and main GUI loop for the Flood Simulator application.
 *
 * Handles window creation, OpenGL context initialization, ImGui setup,
 * and the primary application event loop.
 *
 * \version 1.0
 * \date 2026
 * \copyright Copyright (c) 2026 FloodSim
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

    // Global simulation state variables.
    // Encapsulated in an anonymous namespace to restrict visibility to this translation unit.
    std::atomic<bool> g_is_simulation_running{ false };
    std::shared_ptr<danasim::Application> g_current_simulation = nullptr;
    std::jthread g_simulation_thread;

}  // namespace

namespace floodsim::gui {

    /**
    * \enum ViewState
    * \brief Represents the high-level states of the application's user interface.
    */
    enum class ViewState {
        kHome,          ///< The initial welcome screen.
        kNewScenario,   ///< The view for creating a new scenario from scratch.
        kLoadScenario   ///< The view for working with a loaded scenario.
    };

    void RenderAboutWindow() {
        // Set an elegant initial size
        ImGui::SetNextWindowSize(ImVec2(500, 450), ImGuiCond_FirstUseEver);

        // Usamos una variable local para la 'X' de la esquina superior derecha
        bool is_open = true;

        if (ImGui::BeginPopupModal(("About " + std::string(FLOODSIM_PROGRAM_NAME)).c_str(), &is_open, ImGuiWindowFlags_NoCollapse)) {

            // --- 1. Header & Metadata ---
            ImGui::TextUnformatted((std::string(FLOODSIM_PROGRAM_NAME) + " - Flood Simulation Engine").c_str());
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Version: %s", FLOODSIM_PROGRAM_VERSION);
            ImGui::Text("Copyright (c) %s %s", FLOODSIM_COPYRIGHT_YEAR, FLOODSIM_AUTHOR);
            ImGui::Spacing();

            // --- 2. Project Description ---
            ImGui::TextWrapped(
                "FloodSim is a hydrological simulation engine developed as a Master's Thesis. "
                "It leverages geospatial and meteorological data to model real-time flood behavior "
                "and spatiotemporal dynamic risks."
            );
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // --- 3. Software License ---
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

            // OBLIGATORIO: EndPopup en lugar de End
            ImGui::EndPopup();
        }
    }

    /**
        * \brief Renders the main configuration tabs based on the current configuration.
        *
        * Disables certain tabs while the simulation is actively running to prevent
        * concurrent modification of critical parameters.
        *
        * \param config Reference to the active scenario configuration.
        * \throws std::exception if any sub-rendering function throws an unexpected error.
        */
    void RenderTabs(danasim::Config& config) {
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
                RenderStateUpdaterTab(config.stateUpdater);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Input")) {
                RenderInputTab(config.input, config.stateUpdater.updater);
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
 * \brief Application entry point.
 *
 * Initializes GLFW, OpenGL, and ImGui, and enters the main rendering loop.
 *
 * \param argc Argument count.
 * \param argv Argument vector.
 * \return EXIT_SUCCESS on successful execution, EXIT_FAILURE on fatal errors.
 */
int main(int argc, char** argv) {
    try {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW.");
        }

        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        // ─── COMPATIBILIDAD DE ICONO PARA LINUX ───
        #ifndef _WIN32
        // Define el WM_CLASS para que el Dock de Linux asocie la ventana con el archivo .desktop
        glfwWindowHintString(GLFW_X11_CLASS_NAME, "FloodSimGUI");
        glfwWindowHintString(GLFW_X11_INSTANCE_NAME, "floodsimgui");
        #endif

        GLFWwindow* window = glfwCreateWindow(1280, 720, (std::string(FLOODSIM_PROGRAM_NAME) + " v" + std::string(FLOODSIM_PROGRAM_VERSION)).c_str(), nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window.");
        }

        #ifdef _WIN32
        // Obtener el HWND (manejador de ventana) de GLFW
        HWND hwnd = glfwGetWin32Window(window);

        // Obtener el manejador de la instancia del proceso actual
        HINSTANCE hinstance = GetModuleHandle(NULL);

        // Cargar el icono desde los recursos del archivo .rc usando su nombre/ID
        HICON hIcon = LoadIcon(hinstance, "IDI_ICON1");
        if (hIcon) {
            // Fijar el icono grande (Barra de tareas: Alt+Tab)
            SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            // Fijar el icono pequeño (Esquina de la ventana)
            SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        }
        #endif

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.FontGlobalScale = 1.3f;

        if (!ImGui_ImplGlfw_InitForOpenGL(window, true)) {
            throw std::runtime_error("Failed to initialize ImGui GLFW backend.");
        }
        if (!ImGui_ImplOpenGL3_Init("#version 130")) {
            throw std::runtime_error("Failed to initialize ImGui OpenGL3 backend.");
        }

        floodsim::gui::ViewState current_view = floodsim::gui::ViewState::kHome;
        danasim::Config current_config;
        bool is_dark_theme = true;

        ImGui::StyleColorsDark();

        while (!glfwWindowShouldClose(window)) {
            try {
                glfwPollEvents();

                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                ImGui::SetNextWindowPos(ImVec2(0, 0));
                ImGui::SetNextWindowSize(io.DisplaySize);

                ImGui::Begin("Main Desktop", nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_MenuBar);

                // 1. Declaras la variable bandera (puede ser estática o parte de tu clase/estado)
                static bool trigger_about_popup = false;

                if (ImGui::BeginMenuBar()) {
                    if (ImGui::BeginMenu("Scenario")) {
                        if (ImGui::MenuItem("New Scenario (Blank)")) {
                            current_view = floodsim::gui::ViewState::kNewScenario;
                            current_config = danasim::Config();
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
                                    current_config = danasim::ConfigLoader::load(config_file_path);
                                    current_view = floodsim::gui::ViewState::kLoadScenario;
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
                            trigger_about_popup = true;
                        }
                        ImGui::EndMenu();
                    }

                    ImGui::EndMenuBar();
                }

                if (trigger_about_popup) {
                    ImGui::OpenPopup("About FloodSim");
                    trigger_about_popup = false;
                }

                floodsim::gui::RenderAboutWindow();

                switch (current_view) {
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
                    floodsim::gui::RenderTabs(current_config);
                    break;
                }
                }

                ImGui::End();
                ImGui::Render();

                int display_width, display_height;
                glfwGetFramebufferSize(window, &display_width, &display_height);
                glViewport(0, 0, display_width, display_height);
                glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT);

                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                glfwSwapBuffers(window);

            }
            catch (const std::exception& inner_e) {
                std::cerr << "[Warning] Exception caught during frame render: " << inner_e.what() << "\n";
                // Attempting to finish the frame gracefully to prevent ImGui assertion failures
                ImGui::EndFrame();
            }
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();

        return EXIT_SUCCESS;

    }
    catch (const std::exception& e) {
        std::cerr << "[Fatal Error] " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "[Fatal Error] An unknown error occurred.\n";
        return EXIT_FAILURE;
    }
}
