
#pragma once

// Definir nivel de log activo para compilación (opcional, mejora rendimiento si se define)
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG

#include <spdlog/spdlog.h>
#include <string>
#include <memory>
#include <filesystem>
#include <functional>

#include "logging/LoggerLevel.hpp"

namespace danasim {

    class Logger {
    public:
        // Inicialización global
        static void init(const LoggerLevel& level, bool async, bool silent, bool saveLogFile, const std::filesystem::path& outputPath, std::function<void(int, const std::string&)> guiCallback = nullptr);
        static void shutdown();

        // Obtener el logger crudo de spdlog (para máxima velocidad)
        static std::shared_ptr<spdlog::logger> get();

        // Establecer nombre amigable para la hebra actual
        static void setThreadName(const std::string& name);

    private:
        static spdlog::level::level_enum toSpdLevel(const LoggerLevel& level);
    };

} // namespace danasim

// --- MACROS PARA ACCESO EFICIENTE ---
// Usamos macros para capturar __FILE__ y __LINE__ si fuera necesario en el futuro
// y para evitar evaluación de argumentos si el nivel de log está desactivado.

#define LOG_DEBUG(...)    SPDLOG_LOGGER_DEBUG(danasim::Logger::get(), __VA_ARGS__)
#define LOG_INFO(...)     SPDLOG_LOGGER_INFO(danasim::Logger::get(), __VA_ARGS__)
#define LOG_WARN(...)     SPDLOG_LOGGER_WARN(danasim::Logger::get(), __VA_ARGS__)
#define LOG_ERROR(...)    SPDLOG_LOGGER_ERROR(danasim::Logger::get(), __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(danasim::Logger::get(), __VA_ARGS__)
