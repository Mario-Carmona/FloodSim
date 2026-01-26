/**
 * @file ConfigLoader.hpp
 * @brief Logic for deserializing YAML files into Config objects.
 * @date 2024
 */

#pragma once

#include <filesystem>

#include "app/config/Config.hpp"

namespace danasim {

    /**
     * @class ConfigLoader
     * @brief Static utility class to load and validate application configuration.
     */
    class ConfigLoader {
    public:
        /**
         * @brief Load configuration from a YAML file.
         *
         * Validates the existence of the file and the correctness of the schema.
         *
         * @param filePath The path to the YAML configuration file.
         * @return Config A fully populated and validated configuration object.
         * * @throw ConfigurationException If the file cannot be found or contains invalid data.
         */
        static Config load(const std::filesystem::path& filePath);
    };

} // namespace danasim
