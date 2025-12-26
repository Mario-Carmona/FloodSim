
#pragma once

#include <string>

#include "app/config/Config.hpp"

namespace danasim {

    /**
     * @brief Loads and validates application configuration from YAML files.
     */
    class ConfigLoader {
    public:
        /**
         * @brief Load configuration from a YAML file.
         *
         * @param filePath Path to YAML configuration file
         * @throws std::runtime_error on invalid or missing configuration
         */
        static Config loadFromFile(const std::string& filePath);
    };

} // namespace danasim
