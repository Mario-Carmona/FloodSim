/**
 * @file ConfigLoader.cpp
 * @brief Implementation of the YAML configuration loader.
 */

#include "app/config/ConfigLoader.hpp"

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
            if (value == "protobuf") return OutputConfig::MqttOutputConfigEntry::PayloadFormat::PROTOBUF;
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

            {
                const auto fileNode = requireNode(node, "file");

                config.input.file = InputConfig::FileInputSourceConfig{
                    .staticFormat = extract<std::string>(fileNode, "static_format"),
                    .dynamicFormat = extract<std::string>(fileNode, "dynamic_format"),
                    .path = (configFolder / extract<std::string>(fileNode, "path")).lexically_normal()
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
                        .topic = extract<std::string>(outNode, "topic"),
                        .clientId = extract<std::string>(outNode, "client_id"),
                        .qos = extract<int>(outNode, "qos"),
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
                    .modelPath = (configFolder / extract<std::string>(node, "model_path")).lexically_normal(),
					.tensorDim = extract<int64_t>(node, "tensor_dim")
                };
                break;
            }
            }
        }

        return config;
    }

} // namespace danasim
