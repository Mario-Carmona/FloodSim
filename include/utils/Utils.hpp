
#include <thread>
#include <string>

// Dependencias de sistema
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

void setThreadName(std::thread& thread, const std::string& name) {
#ifdef _WIN32
    // Windows moderno (Windows 10, Server 2016+)
    // Necesitamos convertir std::string (UTF-8/ASCII) a wstring (Wide Char)
    std::wstring wName(name.begin(), name.end());
    HRESULT hr = SetThreadDescription(static_cast<HANDLE>(thread.native_handle()), wName.c_str());
#else
    // Linux / POSIX
    // NOTA: En Linux el nombre está limitado a 15 caracteres + null terminator (16 bytes).
    // Si es más largo, se trunca automáticamente o falla.
    std::string shortName = name.substr(0, 15);
    pthread_setname_np(thread.native_handle(), shortName.c_str());
#endif
}

// Función para nombrar el hilo DESDE EL CUAL SE LLAMA (ej. el main)
void setCurrentThreadName(const std::string& name) {
#ifdef _WIN32
    // Windows requiere Wide Strings (Unicode)
    std::wstring wName(name.begin(), name.end());

    // GetCurrentThread() devuelve un handle al hilo que está ejecutando esta línea
    SetThreadDescription(GetCurrentThread(), wName.c_str());
#else
    // Linux / Mac
    // Limitado a 15 chars en algunas distros
    std::string shortName = name.substr(0, 15);
    pthread_setname_np(pthread_self(), shortName.c_str());
#endif
}
