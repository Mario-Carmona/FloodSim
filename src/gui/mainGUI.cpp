
#include <iostream>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <portable-file-dialogs.h>

#include "app/Application.hpp"

#include "app/config/Config.hpp"
#include "app/config/ConfigLoader.hpp"

#include "gui/tabs/Tabs.hpp"


// Define the possible states of our UI view
enum class ViewState {
    Home,
    NewScenario,
    LoadScenario
};

static std::atomic<bool> isSimulationRunning{ false };
static std::shared_ptr<danasim::Application> currentSimulation = nullptr;
static std::jthread simulationThread;

void renderTabs(danasim::Config& config) {
    ImGui::Spacing();

    // Ahora main_gui tiene acceso directo y nativo a la variable
    bool isRunning = isSimulationRunning.load(std::memory_order_relaxed);

    // --- TAB BAR DEL ESCENARIO ---
    if (ImGui::BeginTabBar("ConfigurationTabs")) {

        // Pestaña 1: General / Control
        if (ImGui::BeginTabItem("Main Control")) {
            FloodSim::Gui::renderMainControlTab(
                config,
                isSimulationRunning,
                currentSimulation,
                simulationThread
            );
            ImGui::EndTabItem();
        }

        // Si la simulación corre, desactivamos el resto
        if (isRunning) { ImGui::BeginDisabled(); }

        // Pestaña 2: Tiempos
        if (ImGui::BeginTabItem("Scenario Info")) {
            FloodSim::Gui::renderScenarioTab(config.scenario);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Simulation")) {
            FloodSim::Gui::renderSimulationTab(config.simulation);
            ImGui::EndTabItem();
        }

        // Pestaña 3: Archivos
        if (ImGui::BeginTabItem("Logging")) {
            FloodSim::Gui::renderLoggingTab(config.logging);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("State Updater")) {
            FloodSim::Gui::renderStateUpdaterTab(config.stateUpdater);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Input")) {
            FloodSim::Gui::renderInputTab(config.input, config.stateUpdater.updater);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Output")) {
            FloodSim::Gui::renderOutputTab(config.output);
            ImGui::EndTabItem();
        }

        if (isRunning) { ImGui::EndDisabled(); }

        ImGui::EndTabBar();
    }
}


int main(int argc, char** argv) {
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "FLOOD SIMULATOR v1.0", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    // --- AUMENTAR TAMAÑO DE LETRA ---
    io.FontGlobalScale = 1.3f;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ViewState currentView = ViewState::Home;

    danasim::Config currentConfig;

    bool isDarkTheme = true;
    ImGui::StyleColorsDark(); // Tema inicial

    while (!glfwWindowShouldClose(window)) {
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

        // --- TOP MENU BAR ---
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Scenario")) {
                if (ImGui::MenuItem("New Scenario (Blank)")) {
                    currentView = ViewState::NewScenario;

                    currentConfig = danasim::Config();
                }

                if (ImGui::MenuItem("Load Existing Configuration...")) {
                    currentView = ViewState::LoadScenario;

                    // Abrimos el diálogo: Título, Directorio inicial ("."), Filtros de extensión
                    auto selection = pfd::open_file("Select Configuration File", ".", { "YAML Files", "*.yaml" }).result();

                    // Si el usuario no canceló, result contiene las rutas elegidas
                    if (!selection.empty()) {
                        std::string configFilePath = selection[0];

                        currentConfig = danasim::ConfigLoader::load(configFilePath);
                    }
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Exit", "Alt+F4")) {
                    glfwSetWindowShouldClose(window, true);
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                // El texto del botón cambia según el tema actual
                if (ImGui::MenuItem(isDarkTheme ? "Switch to Light Theme" : "Switch to Dark Theme")) {
                    isDarkTheme = !isDarkTheme; // Invertimos el estado

                    if (isDarkTheme) {
                        ImGui::StyleColorsDark();
                    }
                    else {
                        ImGui::StyleColorsLight();
                    }
                }
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        // --- VIEW MANAGEMENT ---
        switch (currentView) {
        case ViewState::Home: {
            const char* welcomeText = "Select an option from the 'Scenario' menu to begin.";

            // Aumentamos el tamaño de la fuente al doble temporalmente
            ImGui::SetWindowFontScale(2.0f);

            // Calculamos el tamaño del texto para centrarlo
            ImVec2 textSize = ImGui::CalcTextSize(welcomeText);
            ImVec2 windowSize = ImGui::GetWindowSize();

            // Posicionamos el cursor exactamente en el centro
            ImGui::SetCursorPos(ImVec2((windowSize.x - textSize.x) * 0.5f, (windowSize.y - textSize.y) * 0.5f));

            ImGui::TextDisabled("%s", welcomeText);

            // Restauramos la escala original para no afectar a otros frames
            ImGui::SetWindowFontScale(1.0f);
            break;
        }

        case ViewState::NewScenario: {
            renderTabs(currentConfig);
            break;
        }

        case ViewState::LoadScenario: {
            renderTabs(currentConfig);
            break;
        }

        }

        ImGui::End();

        // --- RENDERING ---
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
