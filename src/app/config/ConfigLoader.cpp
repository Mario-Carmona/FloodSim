/**
 * @file ConfigLoader.cpp
 * @brief Implementation of the YAML configuration loader.
 */

#include "app/config/ConfigLoader.hpp"

#include <algorithm>
#include <yaml-cpp/yaml.h>
#include <string_view>
#include <filesystem>
#include <fmt/core.h>
#include <magic_enum/magic_enum.hpp>

#include "logging/Logger.hpp"
#include "misc/Paths.hpp"
#include "misc/Time.hpp"
#include "core/grid/LayerTypes.hpp"

namespace danasim {

    namespace {

        /**
         * @class ConfigurationException
         * @brief Exception thrown when a configuration error occurs (parsing or validation).
         */
        class ConfigurationException : public std::runtime_error {
        public:
            explicit ConfigurationException(const std::string& message)
                : std::runtime_error("Configuration Error: " + message)
            {
                LOG_ERROR("Configuration Error: {}", message);
            }
        };


        template <typename T>
        T parseEnum(std::string_view value) = delete;

        template <>
        OutputConfig::OutputConfigEntryType parseEnum<OutputConfig::OutputConfigEntryType>(std::string_view value) {
            if (value == "x3d_file") return OutputConfig::OutputConfigEntryType::X3D_FILE;
            if (value == "mqtt")     return OutputConfig::OutputConfigEntryType::MQTT;
            if (value == "image")    return OutputConfig::OutputConfigEntryType::IMAGE;
            throw ConfigurationException("Invalid output type: " + std::string(value));
        }

        template <>
        OutputConfig::MqttOutputConfigEntry::PayloadFormat parseEnum<OutputConfig::MqttOutputConfigEntry::PayloadFormat>(std::string_view value) {
            if (value == "json") return OutputConfig::MqttOutputConfigEntry::PayloadFormat::JSON;
            throw ConfigurationException("Invalid MQTT payload format: " + std::string(value));
        }

        template <>
        UpdaterConfigType parseEnum<UpdaterConfigType>(std::string_view value) {
            if (value == "onnx")   return UpdaterConfigType::ONNX;
            throw ConfigurationException("Invalid state updater type: " + std::string(value));
        }



        // ---------------------------------------------------------
        // YAML Node Extraction Helpers
        // ---------------------------------------------------------

        /**
         * @brief Checks if a YAML node exists, throws descriptive error if not.
         */
        YAML::Node requireNode(const YAML::Node& parent, const std::string& key)
        {
            if (!parent[key]) {
                throw ConfigurationException("Missing required configuration section: '" + key + "'");
            }
            return parent[key];
        }

        /**
         * @brief Safely extracts a value from a node with type checking.
         */
        template <typename T>
        T extract(const YAML::Node& node, const std::string& key)
        {
            if (!node[key]) {
                throw ConfigurationException("Missing required field: '" + key + "'");
            }
            try {
                return node[key].as<T>();
            }
            catch (const YAML::BadConversion&) {
                throw ConfigurationException("Field '" + key + "' has invalid data type.");
            }
        }

        void validate(bool condition, const std::string& message)
        {
            if (!condition) {
                throw ConfigurationException(message);
            }
        }

    } // anonymous namespace

    Config ConfigLoader::load(const std::filesystem::path& configPath)
    {
        if (!std::filesystem::exists(configPath)) {
            throw ConfigurationException("Configuration file not found: " + configPath.string());
        }

        YAML::Node root;
        try {
            root = YAML::LoadFile(configPath.string());
        }
        catch (const YAML::ParserException& e) {
            throw ConfigurationException("YAML Syntax Error: " + std::string(e.what()));
        }

        const std::filesystem::path configFolder = configPath.parent_path();

        Config config;

        // -----------------------------
        // INPUT
        // -----------------------------
        {
            const auto node = requireNode(root, "input");

            {
                const auto fileNode = requireNode(node, "file");

                config.input.file = InputConfig::FileInputSourceConfig{
                    .staticFormat = extract<std::string>(fileNode, "static_format"),
                    .dynamicFormat = extract<std::string>(fileNode, "dynamic_format"),
                    .datasetName = extract<std::string>(fileNode, "dataset_name")
                };
            }

            {
                const auto scalarsNode = requireNode(node, "scalars");

                for (const auto& sclNode : scalarsNode) {
                    config.input.scalars[extract<std::string>(sclNode, "name")] = extract<std::string>(sclNode, "value");
                }
            }
        }

        // -----------------------------
        // OUTPUT
        // -----------------------------
        {
            const auto node = requireNode(root, "output");

            // Snapshot settings
            const auto snapshotNode = requireNode(node, "snapshot");
            config.output.snapshot.everyNSteps = extract<std::size_t>(snapshotNode, "every_n_steps");

            validate(config.output.snapshot.everyNSteps > 0, "snapshot.every_n_steps must be > 0");

            // Output List
            const auto outputsNode = requireNode(node, "outputs");

            for (const auto& outNode : outputsNode) {
                auto type = parseEnum<OutputConfig::OutputConfigEntryType>(extract<std::string>(outNode, "type"));

                switch (type) {
                case OutputConfig::OutputConfigEntryType::X3D_FILE: {
                    config.output.outputs.emplace_back(OutputConfig::X3dFileOutputConfigEntry{});
                    break;
                }
                case OutputConfig::OutputConfigEntryType::MQTT: {
                    config.output.outputs.emplace_back(OutputConfig::MqttOutputConfigEntry{
                        .address = extract<std::string>(outNode, "address"),
                        .payloadFormat = parseEnum<OutputConfig::MqttOutputConfigEntry::PayloadFormat>(extract<std::string>(outNode, "format"))
                        });
                    break;
                }
                case OutputConfig::OutputConfigEntryType::IMAGE: {
                    config.output.outputs.emplace_back(OutputConfig::ImageOutputConfigEntry{});
                    break;
                }
                }
            }

            validate(!config.output.outputs.empty(), "At least one output must be defined");
        }

        // -----------------------------
        // SIMULATION
        // -----------------------------
        {
            const auto node = requireNode(root, "simulation");

            config.simulation.startTimestamp = parseTimestampString(extract<std::string>(node, "start_timestamp"));
            config.simulation.timeStep = parseDurationString(extract<std::string>(node, "time_step"));
            config.simulation.duration = parseDurationString(extract<std::string>(node, "duration"));

            {
                const auto viewNode = requireNode(node, "view_box");

                config.simulation.viewBox.useFullGrid = extract<bool>(viewNode, "use_full_grid");

                if (!config.simulation.viewBox.useFullGrid) {
                    {
                        const auto southWestNode = requireNode(viewNode, "south_west");

                        config.simulation.viewBox.southWest.lon = extract<float>(southWestNode, "lon");
                        config.simulation.viewBox.southWest.lat = extract<float>(southWestNode, "lat");
                    }

                    {
                        const auto northEastNode = requireNode(viewNode, "north_east");

                        config.simulation.viewBox.northEast.lon = extract<float>(northEastNode, "lon");
                        config.simulation.viewBox.northEast.lat = extract<float>(northEastNode, "lat");
                    }
                }
            }

            validate(config.simulation.timeStep > config.simulation.timeStep.zero(), "simulation.time_step must be > 0s");
            validate(config.simulation.duration > config.simulation.duration.zero(), "simulation.duration must be > 0s");
        }

        // -----------------------------
        // LOGGING
        // -----------------------------
        {
            const auto node = requireNode(root, "logging");

            config.logging.level = extract<std::string>(node, "level");
            config.logging.async = extract<bool>(node, "async");
            config.logging.silent = extract<bool>(node, "silent");
            config.logging.saveLogFile = extract<bool>(node, "save_log_file");
        }

        // -----------------------------
        // SCENARIO
        // -----------------------------
        {
            const auto node = requireNode(root, "scenario");

            if (node["output_dir"]) {
                config.scenario.outputDir = (configFolder / extract<std::string>(node, "output_dir")).lexically_normal();
            }
            else {
                config.scenario.outputDir = GetAppDataDirectory("Danasim");
            }

            config.scenario.name = extract<std::string>(node, "name");

            config.scenario.appendStartTimestamp = extract<bool>(node, "append_start_timestamp");
        }

        // -----------------------------
        // STATE UPDATER
        // -----------------------------
        {
            const auto node = requireNode(root, "state_updater");
            

            {
                const auto updaterNode = requireNode(node, "updater");

                auto type = parseEnum<UpdaterConfigType>(extract<std::string>(updaterNode, "type"));

                switch (type) {
                case UpdaterConfigType::ONNX: {
                    config.stateUpdater.updater = OnnxUpdaterConfig{
                        .modelPath = (configFolder / extract<std::string>(updaterNode, "model_path")).lexically_normal(),
                        .tensorDim = extract<int64_t>(updaterNode, "tensor_dim")
                    };
                    break;
                }
                }
            }


			config.stateUpdater.enableRainfall = extract<bool>(node, "enable_rainfall");

            config.stateUpdater.dryTolerance = extract<float>(node, "dry_tolerance");

            {
                const auto floodRiskNode = requireNode(node, "flood_risk");

                const auto defaultLevelNode = requireNode(floodRiskNode, "default_level");

                FloodRiskLevel defaultLevel{
                    .name = extract<std::string>(defaultLevelNode, "name"),
                    .thresholdStart = 0.0,
                    .colorHex = extract<std::string>(defaultLevelNode, "color_hex")
                };


                config.stateUpdater.floodRiskLevels.emplace_back(defaultLevel);


                const auto levelsNode = requireNode(floodRiskNode, "levels");

                for (const auto& elemNode : levelsNode) {
                    config.stateUpdater.floodRiskLevels.emplace_back(FloodRiskLevel{
                        .name = extract<std::string>(elemNode, "name"),
                        .thresholdStart = extract<float>(elemNode, "threshold_start"),
                        .colorHex = extract<std::string>(elemNode, "color_hex")
                    });
                }


                validate(!config.stateUpdater.floodRiskLevels.empty(), "At least one flood risk level must be defined");


                std::sort(
                    config.stateUpdater.floodRiskLevels.begin(),
                    config.stateUpdater.floodRiskLevels.end(),
                    [](const FloodRiskLevel& a, const FloodRiskLevel& b) {
                        return a.thresholdStart < b.thresholdStart;
                    }
                );


                validate(config.stateUpdater.floodRiskLevels[0].name == defaultLevel.name, "First flood risk level must be the default level");
            }
        }

        return config;
    }

} // namespace danasim
