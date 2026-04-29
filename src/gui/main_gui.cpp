
#include <iostream>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include "app/Application.hpp"


int main(int argc, char** argv)
{
    // 1. Inicializar GLFW
    if (!glfwInit()) return 1;

    // Configuración de versión de OpenGL (3.4 Core)
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Simulador de Riadas - Panel de Control", nullptr, nullptr);
    if (!window) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Activar V-Sync

    // 3. Inicializar Contexto de ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Permitir teclado

    // Configurar Estilo (puedes elegir Dark o Light)
    ImGui::StyleColorsDark();

    // 4. Inicializar Backends de ImGui
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Estado de la aplicación
    bool simulacion_en_curso = false;

    // --- Bucle Principal ---
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Iniciar nuevo frame de ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // --- DEFINICIÓN DE LA INTERFAZ ---

        // Creamos una ventana que ocupe toda la pantalla para que parezca una app nativa
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::Begin("Panel Principal", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        ImGui::Text("Simulador de Riadas v1.0");
        ImGui::Separator();
        ImGui::Spacing();

        // Botón principal de ejecución
        // Lo hacemos un poco más grande y de color verde
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.7f, 0.2f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.8f, 0.3f, 1.0f));

        if (ImGui::Button("ARRANCAR SIMULACIÓN", ImVec2(250, 50))) {
            simulacion_en_curso = true;
        }

        ImGui::PopStyleColor(2);

        if (simulacion_en_curso) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Estado: Simulando...");
        }
        else {
            ImGui::Text("Estado: Esperando configuración");
        }

        ImGui::End(); // Fin de "Panel Principal"

        // --- RENDERIZADO ---
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // --- LIMPIEZA ---
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
