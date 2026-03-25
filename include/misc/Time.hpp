
#pragma once

#include <iostream>
#include <chrono>
#include <sstream>
#include <string>
#include <iomanip> 
#include <ctime> 
#include <stdexcept>

inline std::chrono::sys_seconds parseTimestampString(const std::string& s) {
    std::istringstream ss{ s };
    std::tm tm = {};

    // %Y-%m-%dT%H:%M:%S es el equivalente manual a %FT%T
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");

    if (ss.fail()) {
        throw std::runtime_error("Fallo al parsear el timestamp: " + s);
    }

    // Compatibilidad multiplataforma para convertir tm a time_t (UTC)
#if defined(_WIN32) || defined(_WIN64)
    time_t time = _mkgmtime(&tm); // Versión de Windows (MSVC)
#else
    time_t time = timegm(&tm);    // Versión de Linux/macOS (POSIX)
#endif

    // Convertimos time_t al formato chrono sys_seconds esperado
    return std::chrono::time_point_cast<std::chrono::seconds>(
        std::chrono::system_clock::from_time_t(time)
    );
}

inline std::chrono::seconds parseDurationString(const std::string& s) {
    std::istringstream ss{ s };
    int hours = 0, minutes = 0, seconds = 0;
    char colon1, colon2;

    // Extraemos las horas, minutos y segundos ignorando los caracteres ':'
    ss >> hours >> colon1 >> minutes >> colon2 >> seconds;

    if (ss.fail() || colon1 != ':' || colon2 != ':') {
        throw std::runtime_error("Fallo al parsear la duracion: " + s);
    }

    return std::chrono::hours(hours) + std::chrono::minutes(minutes) + std::chrono::seconds(seconds);
}

