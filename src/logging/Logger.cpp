
#include "logging/Logger.hpp"

#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/pattern_formatter.h>

#include <mutex>
#include <shared_mutex> // C++17
#include <unordered_map>
#include <thread>
#include <atomic> // Necesario para la longitud dinámica sin bloqueos

namespace danasim {

    // --- ALMACENAMIENTO DE NOMBRES DE HEBRAS ---
    // Usamos un mapa estático: ThreadID -> Nombre
    static std::unordered_map<size_t, std::string> s_threadNames;
    // Mutex de lectura/escritura para acceso ultra-rápido en lecturas concurrentes
    static std::shared_mutex s_threadNamesMutex;

    // 2. Variable atómica para la longitud máxima (iniciamos con un mínimo, ej. 8)
    static std::atomic<size_t> s_maxThreadNameWidth{ 8 };

    static std::shared_ptr<spdlog::logger> s_logger;

    // --- Custom Flag con Alineación Dinámica ---
    class ThreadNameFlag : public spdlog::custom_flag_formatter
    {
    public:
        void format(const spdlog::details::log_msg& msg, const std::tm&, spdlog::memory_buf_t& dest) override
        {
            std::string_view name = "Unknown";

            // Leemos el nombre (bloqueo compartido, muy rápido)
            {
                std::shared_lock<std::shared_mutex> lock(s_threadNamesMutex);
                auto it = s_threadNames.find(msg.thread_id);
                if (it != s_threadNames.end()) {
                    name = it->second;
                }
            }

            // 3. Obtenemos el ancho actual máximo de forma atómica
            size_t width = s_maxThreadNameWidth.load(std::memory_order_relaxed);

            // 4. Formateo dinámico:
            // "{:<{}}" se desglosa así:
            //    :   -> Inicio de especificaciones de formato
            //    <   -> Alinear a la izquierda
            //    {}  -> El hueco para el ancho (toma el argumento 'width')
            spdlog::fmt_lib::format_to(std::back_inserter(dest), "{:<{}}", name, width);
        }

        std::unique_ptr<custom_flag_formatter> clone() const override
        {
            return spdlog::details::make_unique<ThreadNameFlag>();
        }
    };

    // Helper: Convertir string level a enum
    static spdlog::level::level_enum toSpd(const std::string& level) {
        if (level == "trace") return spdlog::level::trace;
        if (level == "debug") return spdlog::level::debug;
        if (level == "info")  return spdlog::level::info;
        if (level == "warn")  return spdlog::level::warn;
        if (level == "error") return spdlog::level::err;
        if (level == "critical") return spdlog::level::critical;
        return spdlog::level::info;
    }

    void Logger::init(const std::string& level, bool async, bool silent, bool saveLogFile, const std::filesystem::path& outputPath)
    {
        std::vector<spdlog::sink_ptr> sinks;

        if (!silent) {
            sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

            if (saveLogFile) {
                std::filesystem::path logFile = outputPath / "simulation.log";

                sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile.string(), true));
            }
        }
        else {
            sinks.push_back(std::make_shared<spdlog::sinks::null_sink_mt>());
        }

        if (async && !silent) {
            spdlog::init_thread_pool(8192, 1);
            s_logger = std::make_shared<spdlog::async_logger>(
                "danasim", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
        }
        else {
            s_logger = std::make_shared<spdlog::logger>("danasim", sinks.begin(), sinks.end());
        }

        s_logger->set_level(toSpd(level));

        // Configurar formatter
        auto formatter = std::make_unique<spdlog::pattern_formatter>();
        formatter->add_flag<ThreadNameFlag>('*');
        formatter->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%*] [%^%l%$] %v");
        s_logger->set_formatter(std::move(formatter));

        spdlog::set_default_logger(s_logger);

        // Registrar el hilo principal automáticamente
        setThreadName("Main");
    }

    void Logger::shutdown() {
        spdlog::shutdown();
    }

    std::shared_ptr<spdlog::logger> Logger::get() {
        return s_logger;
    }

    void Logger::setThreadName(const std::string& name) {
        size_t threadId = spdlog::details::os::thread_id();

        // Guardar nombre en el mapa
        {
            std::unique_lock<std::shared_mutex> lock(s_threadNamesMutex);
            s_threadNames[threadId] = name;
        }

        // 5. Actualizar el ancho máximo si el nuevo nombre es más largo.
        // Usamos un bucle CAS (Compare-And-Swap) para thread-safety sin mutex.
        size_t len = name.length();
        size_t currentMax = s_maxThreadNameWidth.load(std::memory_order_relaxed);

        while (len > currentMax) {
            // Intentamos actualizar currentMax a len.
            // Si otro hilo cambió currentMax mientras tanto, esto falla, 
            // actualiza currentMax con el valor real y reintentamos.
            if (s_maxThreadNameWidth.compare_exchange_weak(currentMax, len)) {
                break; // Actualización exitosa
            }
        }
    }

} // namespace danasim
