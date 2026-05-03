
#include "gui/tabs/Tabs.hpp"

#include <portable-file-dialogs.h>

#include "app/Application.hpp"
#include "app/config/Config.hpp"
#include "app/config/ConfigLoader.hpp"

namespace FloodSim::Gui {

    struct AppLog {
        // Ahora guardamos las tres partes de la línea por separado
        struct LogMessage {
            std::string Prefix;     // Contendrá: "[2026-05-02 19:41...] [Core    ] "
            std::string LevelTag;   // Contendrá: "[info]"
            std::string Payload;    // Contendrá: " Configuring ONNX Runtime..."
            ImVec4 Color;
        };

        std::vector<LogMessage> Items;
        std::mutex LogMutex;

        bool AutoScroll = true;       // Controla el comportamiento inteligente
        bool ScrollToBottom = false;  // Disparador manual para forzar la bajada (un solo uso)

        void Clear() {
            std::lock_guard<std::mutex> lock(LogMutex);
            Items.clear();
        }

        void AddLog(int spdlogLevel, const std::string& rawMsg) {
            // 1. spdlog siempre añade un '\n' al final del mensaje formateado.
            // En ImGui no lo queremos porque nos crearía líneas en blanco extra.
            std::string msg = rawMsg;
            if (!msg.empty() && msg.back() == '\n') {
                msg.pop_back();
            }

            // 2. Mapeamos los colores
            ImVec4 color;
            switch (spdlogLevel) {
            case 1: color = ImVec4(0.2f, 0.6f, 1.0f, 1.0f); break; // Debug (Azul)
            case 2: color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f); break; // Info (Verde)
            case 3: color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f); break; // Warn (Amarillo)
            case 4: color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); break; // Error (Rojo)
            case 5: color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f); break; // Critical (Rojo intenso)
            default: color = ImVec4(0.9f, 0.9f, 0.9f, 1.0f); break;
            }

            // 3. PARSEO DEL PATRÓN: Buscamos el tercer par de corchetes
            size_t pos1 = msg.find('[');
            size_t pos2 = msg.find('[', pos1 + 1);
            size_t pos3 = msg.find('[', pos2 + 1);
            size_t pos3_close = msg.find(']', pos3 + 1);

            std::string prefix, tag, payload;

            if (pos3 != std::string::npos && pos3_close != std::string::npos) {
                // Lo dividimos con éxito
                prefix = msg.substr(0, pos3);                  // Desde el inicio hasta el 3º '['
                tag = msg.substr(pos3, pos3_close - pos3 + 1); // Exactamente "[info]" o "[debug]"
                payload = msg.substr(pos3_close + 1);          // Todo lo demás
            }
            else {
                // Fallback de seguridad por si envías un log que no sigue el patrón
                prefix = "";
                tag = "";
                payload = msg;
            }

            std::lock_guard<std::mutex> lock(LogMutex);
            Items.push_back({ prefix, tag, payload, color });
        }

        void Draw(const char* title) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "Terminal Output");
            ImGui::Spacing();

            if (ImGui::Checkbox("Auto-scroll", &AutoScroll)) {
                // Si el nuevo valor es true (se acaba de activar), forzamos la bajada
                if (AutoScroll) {
                    ScrollToBottom = true;
                }
            }

            ImGui::SameLine();

            if (ImGui::Button("Clear Log")) {
                Clear();
            }

            ImGui::BeginChild(title, ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

            {
                std::lock_guard<std::mutex> lock(LogMutex);
                for (const auto& item : Items) {
                    if (!item.Prefix.empty()) {
                        // 1. Dibujamos el prefijo (Fecha + Hilo) con el color grisáceo por defecto
                        ImGui::TextDisabled("%s", item.Prefix.c_str());

                        // 2. Le decimos a ImGui que dibuje lo siguiente pegado a lo anterior
                        ImGui::SameLine(0, 0);

                        // 3. Dibujamos la etiqueta con su color especial (verde, amarillo...)
                        ImGui::PushStyleColor(ImGuiCol_Text, item.Color);
                        ImGui::TextUnformatted(item.LevelTag.c_str());
                        ImGui::PopStyleColor();

                        ImGui::SameLine(0, 0);

                        // 4. Dibujamos el texto del mensaje con el color normal de la interfaz
                        ImGui::TextUnformatted(item.Payload.c_str());
                    }
                    else {
                        // Fallback: Si no pudimos separarlo, lo pintamos todo del color del nivel
                        ImGui::PushStyleColor(ImGuiCol_Text, item.Color);
                        ImGui::TextUnformatted(item.Payload.c_str());
                        ImGui::PopStyleColor();
                    }
                }
            }

            // --- LÓGICA DE SCROLL INTELIGENTE ---
            // Si forzamos el scroll manual O (el auto-scroll está activo Y estamos al final de la ventana)
            if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())) {
                ImGui::SetScrollHereY(1.0f);
            }

            // ¡Crucial! Reseteamos el disparador manual para que no bloquee el ratón en el siguiente frame
            ScrollToBottom = false;

            ImGui::EndChild();
        }
    };

    // Variable estática para persistir los logs durante la ejecución
    static AppLog guiConsole;

    void renderMainControlTab(const danasim::Config& config, std::atomic<bool>& isSimulationRunning,
            std::shared_ptr<danasim::Application>& currentSimulation, std::jthread& simulationThread) {
        
        ImGui::Spacing();

        // --- SECCIÓN 2: GESTIÓN DE CONFIGURACIÓN ---
        ImGui::SeparatorText("Configuration Management");
        ImGui::Spacing();

        if (ImGui::Button("Save Configuration As...")) {
            auto destination = pfd::save_file("Save Scenario Config", "", { "YAML Files", "*.yaml" }).result();
            if (!destination.empty()) {
                danasim::ConfigLoader::saveToFile(destination, config);
            }
        }

        ImGui::Spacing();
        ImGui::Spacing();

        // --- SECCIÓN 1: BOTONES DE CONTROL ---
        ImGui::SeparatorText("Simulation Control");
        ImGui::Spacing();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

        // 1. Capturamos el estado actual al inicio del frame
        bool isRunning = isSimulationRunning.load(std::memory_order_relaxed);

        // --- BOTÓN START ---
        if (isRunning) { ImGui::BeginDisabled(); }

        // Botón START
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        if (ImGui::Button("START SIMULATION", ImVec2(180, 45))) {
            guiConsole.Clear();
            
            // Cambiamos el estado, pero la variable local 'isRunning' sigue siendo false este frame
            isSimulationRunning.store(true, std::memory_order_relaxed);

            // 1. Preparamos el Callback
            auto logToGui = [](int level, const std::string& msg) {
                guiConsole.AddLog(level, msg);
            };

            // 2. Instanciamos el motor
            currentSimulation = std::make_shared<danasim::Application>(config, logToGui);

            // 3. Lanzamos el hilo secundario
            simulationThread = std::jthread([&currentSimulation, &isSimulationRunning]() {
                // Este bloque de código corre en su propio mundo sin afectar la GUI
                if (currentSimulation) {
                    currentSimulation->run(); // Arranca el ciclo pesado
                }

                // Cuando start() termina, avisamos de que hemos acabado
                isSimulationRunning.store(false, std::memory_order_relaxed);
            });
        }
        ImGui::PopStyleColor();

        if (isRunning) { ImGui::EndDisabled(); }

        ImGui::SameLine();

        // Botón STOP
        // Aquí la lógica es inversa: si NO está corriendo, lo deshabilitamos
        if (!isRunning) { ImGui::BeginDisabled(); }

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
        if (ImGui::Button("STOP", ImVec2(120, 45))) {
            if (currentSimulation) {
                currentSimulation->stop();
            }
        }
        ImGui::PopStyleColor();

        if (!isRunning) { ImGui::EndDisabled(); }

        ImGui::PopStyleVar();

        ImGui::Spacing();
        ImGui::Spacing();

        // --- SECCIÓN 3: CONSOLA DE SALIDA ---
        // Usamos el resto del espacio disponible para la consola
        guiConsole.Draw("ConsoleWindow");
    }

}
