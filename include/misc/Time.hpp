
#pragma once

#include <iostream>
#include <chrono>
#include <sstream>
#include <string>

inline std::chrono::sys_seconds parseTimestampString(const std::string& s) {
    std::istringstream ss{ s };
    std::chrono::sys_seconds tp; // sys_time con precisión de segundos

    // %F es equivalente a %Y-%m-%d
    // %T es equivalente a %H:%M:%S
    ss >> std::chrono::parse("%FT%T", tp);

    if (ss.fail()) {
        throw std::runtime_error("Fallo al parsear el timestamp: " + s);
    }

    return tp;
}

inline std::chrono::seconds parseDurationString(const std::string& s) {
    std::istringstream ss{ s };
    std::chrono::seconds sec;

    // %T parsea el formato HH:MM:SS y lo convierte automáticamente a la duración total
    ss >> std::chrono::parse("%T", sec);

    return sec;
}

