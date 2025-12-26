
#include "app/config/ConfigLoader.hpp"

#include <yaml-cpp/yaml.h>
#include <stdexcept>
#include <string>

namespace danasim {

    namespace {

        /* ---------- Helper functions ---------- */

        InputConfigType parseInputType(const std::string& value)
        {
            if (value == "file") return InputConfigType::FILE;
            if (value == "mqtt") return InputConfigType::MQTT;

            throw std::runtime_error("Invalid input type: " + value);
        }

        OutputConfig::OutputConfigEntryType parseOutputType(const std::string& value)
        {
            if (value == "x3d_file") return OutputConfig::OutputConfigEntryType::X3D_FILE;
            if (value == "mqtt") return OutputConfig::OutputConfigEntryType::MQTT;

            throw std::runtime_error("Invalid output type: " + value);
        }

        OutputConfig::MQTTFormat parseOutputFormat(const std::string& value)
        {
            if (value == "protobuf") return OutputConfig::MQTTFormat::PROTOBUF;

            throw std::runtime_error("Invalid snapshot serializer format: " + value);
        }

        StateUpdaterConfigType parseStateUpdaterType(const std::string& value)
        {
            if (value == "tensorflow") return StateUpdaterConfigType::TENSORFLOW;
            if (value == "experimental") return StateUpdaterConfigType::EXPERIMENTAL;

            throw std::runtime_error("Invalid state updater type: " + value);
        }

        void requireNode(const YAML::Node& node, const std::string& name)
        {
            if (!node) {
                throw std::runtime_error("Missing required configuration section: " + name);
            }
        }

        void requireValue(bool condition, const std::string& message)
        {
            if (!condition) {
                throw std::runtime_error(message);
            }
        }

    } // anonymous namespace

    Config ConfigLoader::loadFromFile(const std::string& filePath)
    {
        YAML::Node root = YAML::LoadFile(filePath);

        Config config;

        /* =============================
         * INPUT
         * ============================= */
        requireNode(root["input"], "input");
        {
            const auto& node = root["input"];

            requireNode(node["type"], "input.type");
            InputConfigType type = parseInputType(node["type"].as<std::string>());

            switch (type) {

            case InputConfigType::FILE: {
                requireNode(node["path"], "input.path");

                config.input = FileInputConfig {
                    .path = node["path"].as<std::string>()
                };

                break;
            }

            case InputConfigType::MQTT: {
                requireNode(node["broker"], "input.broker");
                requireNode(node["topic"], "input.topic");

                config.input = MQTTInputConfig {
                    .broker = node["broker"].as<std::string>(),
                    .topic = node["topic"].as<std::string>()
				};

                break;
            }
            }
        }

        /* =============================
         * OUTPUT
         * ============================= */
        requireNode(root["output"], "output");
        {
            const auto& node = root["output"];

            // Snapshot policy
            requireNode(node["snapshot"], "output.snapshot");
            {
                const auto& snapshotNode = node["snapshot"];

                requireNode(snapshotNode["every_n_steps"], "snapshot.every_n_steps");

                config.output.snapshot = OutputConfig::SnapshotConfig {
                    .everyNSteps = snapshotNode["every_n_steps"].as<std::size_t>()
                };

                requireValue(
                    config.output.snapshot.everyNSteps > 0,
                    "snapshot.every_n_steps must be > 0"
                );
            }

            // Outputs
            requireNode(node["outputs"], "output.outputs");

            for (const auto& outNode : node["outputs"]) {
                requireNode(outNode["type"], "output.outputs[].type");
                OutputConfig::OutputConfigEntryType type = parseOutputType(outNode["type"].as<std::string>());

                switch (type) {

                case OutputConfig::OutputConfigEntryType::X3D_FILE: {
                    requireNode(outNode["file_path"], "output.outputs[].file_path");

                    config.output.outputs.emplace_back(OutputConfig::X3DFileOutputConfigEntry{
                        .filePath = outNode["file_path"].as<std::string>()
                    });

                    break;
                }

                case OutputConfig::OutputConfigEntryType::MQTT: {
                    requireNode(outNode["address"], "output.outputs[].address");
                    requireNode(outNode["topic"], "output.outputs[].topic");
                    requireNode(outNode["client_id"], "output.outputs[].client_id");
                    requireNode(outNode["qos"], "output.outputs[].qos");
                    requireNode(outNode["format"], "output.outputs[].format");

                    config.output.outputs.emplace_back(OutputConfig::MQTTOutputConfigEntry{
                        .address = outNode["address"].as<std::string>(),
                        .topic = outNode["topic"].as<std::string>(),
                        .clientId = outNode["client_id"].as<std::string>(),
                        .qos = outNode["qos"].as<int>(),
                        .format = parseOutputFormat(outNode["format"].as<std::string>())
                    });

                    break;
                }
                }
            }

            requireValue(
                !config.output.outputs.empty(),
                "At least one output must be defined"
            );
        }

        /* =============================
         * SIMULATION
         * ============================= */
        requireNode(root["simulation"], "simulation");
        {
            const auto& node = root["simulation"];

            requireNode(node["time_step"], "simulation.time_step");
            requireNode(node["total_time"], "simulation.total_time");

            config.simulation.timeStep = node["time_step"].as<float>();
            config.simulation.totalTime = node["total_time"].as<float>();

            requireValue(
                config.simulation.timeStep > 0.0,
                "simulation.time_step must be > 0"
            );

            requireValue(
                config.simulation.totalTime >= 0.0,
                "simulation.total_time must be >= 0"
            );
        }

        requireNode(root["logging"], "logging");
        {
            const auto& node = root["logging"];

            requireNode(node["level"], "simulation.level");
            requireNode(node["async"], "simulation.async");
            requireNode(node["silent"], "simulation.silent");

            config.logging.level = node["level"].as<std::string>();
            config.logging.async = node["async"].as<bool>();
            config.logging.silent = node["silent"].as<bool>();
            config.logging.file = node["file"].as<std::string>("");
        }

        requireNode(root["scenario"], "scenario");
        {
            const auto& node = root["scenario"];

            requireNode(node["name"], "scenario.name");

            config.scenario.name = node["name"].as<std::string>();
        }

        requireNode(root["state_updater"], "state_updater");
        {
            const auto& node = root["state_updater"];

            requireNode(node["type"], "state_updater.type");
            StateUpdaterConfigType type = parseStateUpdaterType(node["type"].as<std::string>());

            switch (type) {

                case StateUpdaterConfigType::TENSORFLOW: {
                    requireNode(node["model_path"], "state_updater.model_path");

                    config.stateUpdater = TensorFlowStateUpdaterConfig{
                        .modelPath = node["model_path"].as<std::string>()
                    };

                    break;
                }

                case StateUpdaterConfigType::EXPERIMENTAL: {
                    config.stateUpdater = ExperimentalStateUpdaterConfig{};

                    break;
                }
            }
        }

        return config;
    }

} // namespace danasim
