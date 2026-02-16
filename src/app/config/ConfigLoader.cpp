/**
 * @file ConfigLoader.cpp
 * @brief Implementation of the YAML configuration loader.
 */

#include "app/config/ConfigLoader.hpp"

#include <yaml-cpp/yaml.h>
#include <string_view>
#include <filesystem>
#include <fmt/core.h>

#include "logging/Logger.hpp"
#include "misc/Paths.hpp"

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
        InputConfigType parseEnum<InputConfigType>(std::string_view value) {
            if (value == "file") return InputConfigType::FILE;
            throw ConfigurationException("Invalid input type: " + std::string(value));
        }

        template <>
        OutputConfig::OutputConfigEntryType parseEnum<OutputConfig::OutputConfigEntryType>(std::string_view value) {
            if (value == "x3d_file") return OutputConfig::OutputConfigEntryType::X3D_FILE;
            if (value == "mqtt")     return OutputConfig::OutputConfigEntryType::MQTT;
            if (value == "image")    return OutputConfig::OutputConfigEntryType::IMAGE;
            throw ConfigurationException("Invalid output type: " + std::string(value));
        }

        template <>
        OutputConfig::MQTTOutputConfigEntry::PayloadFormat parseEnum<OutputConfig::MQTTOutputConfigEntry::PayloadFormat>(std::string_view value) {
            if (value == "protobuf") return OutputConfig::MQTTOutputConfigEntry::PayloadFormat::PROTOBUF;
            throw ConfigurationException("Invalid MQTT payload format: " + std::string(value));
        }

        template <>
        StateUpdaterConfigType parseEnum<StateUpdaterConfigType>(std::string_view value) {
            if (value == "onnx")   return StateUpdaterConfigType::ONNX;
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

            InputConfigType type = parseEnum<InputConfigType>(extract<std::string>(node, "type"));

            switch (type) {
            case InputConfigType::FILE: {
                config.input = FileInputConfig{
                    .path = (configFolder / extract<std::string>(node, "path")).lexically_normal()
                };
                break;
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
                    config.output.outputs.emplace_back(OutputConfig::X3DFileOutputConfigEntry{});
                    break;
                }
                case OutputConfig::OutputConfigEntryType::MQTT: {
                    config.output.outputs.emplace_back(OutputConfig::MQTTOutputConfigEntry{
                        .address = extract<std::string>(outNode, "address"),
                        .topic = extract<std::string>(outNode, "topic"),
                        .clientId = extract<std::string>(outNode, "client_id"),
                        .qos = extract<int>(outNode, "qos"),
                        .payloadFormat = parseEnum<OutputConfig::MQTTOutputConfigEntry::PayloadFormat>(extract<std::string>(outNode, "format"))
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

            config.simulation.timeStep = extract<float>(node, "time_step");
            config.simulation.totalTime = extract<float>(node, "total_time");

            config.simulation.viewMinX = extract<float>(node, "view_min_x");
            config.simulation.viewMaxX = extract<float>(node, "view_max_x");
            config.simulation.viewMinY = extract<float>(node, "view_min_y");
            config.simulation.viewMaxY = extract<float>(node, "view_max_y");

            validate(config.simulation.timeStep > 0.0f, "simulation.time_step must be > 0");
            validate(config.simulation.totalTime >= 0.0f, "simulation.total_time must be >= 0");
        }

        // -----------------------------
        // LOGGING
        // -----------------------------
        {
            const auto node = requireNode(root, "logging");

            config.logging.level = extract<std::string>(node, "level");
            config.logging.async = extract<bool>(node, "async");
            config.logging.silent = extract<bool>(node, "silent");
            config.logging.saveLogFile = extract<bool>(node, "saveLogFile");
        }

        // -----------------------------
        // SCENARIO
        // -----------------------------
        {
            const auto node = requireNode(root, "scenario");

            if (node["outputDir"]) {
                config.scenario.outputDir = (configFolder / extract<std::string>(node, "outputDir")).lexically_normal();
            }
            else {
                config.scenario.outputDir = GetAppDataDirectory("Danasim");
            }

            config.scenario.name = extract<std::string>(node, "name");
        }

        // -----------------------------
        // STATE UPDATER
        // -----------------------------
        {
            const auto node = requireNode(root, "state_updater");
            
            auto type = parseEnum<StateUpdaterConfigType>(extract<std::string>(node, "type"));

            switch (type) {
            case StateUpdaterConfigType::ONNX: {
                config.stateUpdater = OnnxStateUpdaterConfig{
                    .modelPath = (configFolder / extract<std::string>(node, "model_path")).lexically_normal()
                };
                break;
            }
            }
        }

        return config;
    }

} // namespace danasim
