
#pragma once

#include <string>
#include <filesystem>
#include "whereami.h" // Ajusta la ruta si es necesario

namespace danasim {

    // Función para obtener la ruta completa del ejecutable (.exe)
    inline std::filesystem::path GetExecutablePath() {
        // 1. Llamamos a la función con NULL para saber cuántos caracteres ocupa la ruta
        int length = wai_getExecutablePath(NULL, 0, NULL);
        if (length <= 0) {
            return std::filesystem::path(); // Error: devolvemos ruta vacía
        }

        // 2. Reservamos un std::string con el tamaño exacto
        std::string path(length, '\0');

        // 3. Volvemos a llamar a la función para rellenar nuestro string
        // El último parámetro (NULL) es por si queremos que nos diga dónde acaba la carpeta,
        // pero nosotros usaremos std::filesystem para eso.
        wai_getExecutablePath(&path[0], length, NULL);

        // 4. Devolvemos la ruta ya formateada para C++17
        return std::filesystem::path(path);
    }

    // Función auxiliar para obtener directamente la carpeta del ejecutable
    inline std::filesystem::path GetExecutableFolder() {
        return GetExecutablePath().parent_path();
    }

    /**
 * @brief Obtiene el directorio estándar del sistema operativo para guardar datos de la aplicación.
 * @param appName El nombre de tu aplicación (ej: "Danasim") para crear una subcarpeta dedicada.
 * @return std::filesystem::path La ruta absoluta al directorio de datos de la app.
 */
    inline std::filesystem::path GetAppDataDirectory(const std::string& appName)
    {
        std::filesystem::path appDataPath;

#if defined(_WIN32)
        // --- WINDOWS ---
        const char* appData = std::getenv("APPDATA");
        if (appData != nullptr) {
            appDataPath = std::filesystem::path(appData);
        }
        else {
            // Fallback poco probable en Windows, pero seguro
            appDataPath = std::filesystem::current_path();
        }

#elif defined(__APPLE__)
        // --- macOS ---
        const char* home = std::getenv("HOME");
        if (home != nullptr) {
            appDataPath = std::filesystem::path(home) / "Library" / "Application Support";
        }

#elif defined(__linux__) || defined(__unix__)
        // --- LINUX / UNIX ---
        const char* xdgDataHome = std::getenv("XDG_DATA_HOME");
        if (xdgDataHome != nullptr && std::string(xdgDataHome).length() > 0) {
            appDataPath = std::filesystem::path(xdgDataHome);
        }
        else {
            // Fallback estándar XDG si la variable no está definida
            const char* home = std::getenv("HOME");
            if (home != nullptr) {
                appDataPath = std::filesystem::path(home) / ".local" / "share";
            }
        }
#else
#error "Sistema operativo no soportado"
#endif

        // Si logramos obtener una ruta base válida, le añadimos la carpeta de nuestra app
        if (!appDataPath.empty() && !appName.empty()) {
            appDataPath /= appName;
        }

        return appDataPath;
    }

} // namespace danasim
