/**
 * @file ConfigLoader.hpp
 * @brief Logic for deserializing YAML files into Config objects.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <filesystem>

#include "app/config/Config.hpp"

namespace floodsim {

/**
 * @class ConfigLoader
 * @brief Static utility class to load, validate, and save application configuration.
 */
class ConfigLoader {
public:
    /**
     * @brief Load configuration from a YAML file.
     *
     * Validates the existence of the file and the correctness of the schema.
     *
     * @param config_path The path to the YAML configuration file.
     * @return Config A fully populated and validated configuration object.
     * @throw ConfigurationException If the file cannot be found, contains invalid data, or fails to parse.
     */
    static Config Load(const std::filesystem::path& config_path);

    /**
     * @brief Save a configuration object to a YAML file.
     *
     * @param destination The output path where the YAML will be written.
     * @param config The populated Config object to serialize.
     * @throw ConfigurationException If the file cannot be written or serialization fails.
     */
    static void SaveToFile(const std::filesystem::path& destination, const Config& config);
};

} // namespace floodsim
