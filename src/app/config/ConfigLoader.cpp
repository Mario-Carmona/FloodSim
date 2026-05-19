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
#include <fmt/chrono.h>
#include <fstream>

#include "logging/LoggerLevel.hpp"
#include "misc/Paths.hpp"
#include "misc/Time.hpp"
#include "app/core/grid/LayerTypes.hpp"

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
                
            }
        };



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
                    .staticFormat = magic_enum::enum_cast<FileStaticFormat>(extract<std::string>(fileNode, "static_format"), magic_enum::case_insensitive).value(),
                    .dynamicFormat = magic_enum::enum_cast<FileDynamicFormat>(extract<std::string>(fileNode, "dynamic_format"), magic_enum::case_insensitive).value(),
					.datasetFolder = (configFolder / extract<std::string>(fileNode, "dataset_folder")).lexically_normal(),
                    .datasetName = extract<std::string>(fileNode, "dataset_name")
                };
            }

            {
                const auto scalarsNode = requireNode(node, "scalars");

                for (const auto& it : scalarsNode) {
                    // it.first es la clave, it.second es el valor
                    std::string key = it.first.as<std::string>();
                    std::string value = it.second.as<std::string>();

                    config.input.scalars[key] = value;
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
                OutputConfig::OutputConfigEntryType type = magic_enum::enum_cast<OutputConfig::OutputConfigEntryType>(extract<std::string>(outNode, "type"), magic_enum::case_insensitive).value();

                switch (type) {
                case OutputConfig::OutputConfigEntryType::CHECKPOINT: {
                    config.output.outputs.emplace_back(OutputConfig::CheckpointOutputConfigEntry{
                        .staticFormat = magic_enum::enum_cast<FileStaticFormat>(extract<std::string>(outNode, "static_format"), magic_enum::case_insensitive).value()
                        });
                    break;
                }
                case OutputConfig::OutputConfigEntryType::MQTT: {
                    config.output.outputs.emplace_back(OutputConfig::MqttOutputConfigEntry{
                        .protocol = extract<std::string>(outNode, "protocol"),
                        .host = extract<std::string>(outNode, "host"),
                        .port = extract<int>(outNode, "port"),
                        .payloadFormat = magic_enum::enum_cast<OutputConfig::MqttOutputConfigEntry::PayloadFormat>(extract<std::string>(outNode, "format"), magic_enum::case_insensitive).value()
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

            config.logging.level = magic_enum::enum_cast<LoggerLevel>(extract<std::string>(node, "level"), magic_enum::case_insensitive).value();
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

                UpdaterConfigType type = magic_enum::enum_cast<UpdaterConfigType>(extract<std::string>(updaterNode, "type"), magic_enum::case_insensitive).value();

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

    // Helper para std::visit (si no lo tienes ya definido a nivel global o en este archivo)
    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    void ConfigLoader::saveToFile(const std::filesystem::path& destination, const Config& config) {
        YAML::Node root;

        // ---------------------------------------------------------
        // 1. Logging Config
        // ---------------------------------------------------------
        YAML::Node loggingNode;
        loggingNode["level"] = std::string(magic_enum::enum_name(config.logging.level));
        loggingNode["async"] = config.logging.async;
        loggingNode["silent"] = config.logging.silent;
        loggingNode["save_log_file"] = config.logging.saveLogFile;
        root["logging"] = loggingNode;

        // ---------------------------------------------------------
        // 2. Scenario Config
        // ---------------------------------------------------------
        YAML::Node scenarioNode;
        scenarioNode["name"] = config.scenario.name;
        scenarioNode["output_dir"] = config.scenario.outputDir.string();
        scenarioNode["append_start_timestamp"] = config.scenario.appendStartTimestamp;
        root["scenario"] = scenarioNode;

        // ---------------------------------------------------------
        // 3. Simulation Config
        // ---------------------------------------------------------
        YAML::Node simNode;
        // Guardar timestamp usando fmt::chrono o como lo maneje tu misc/Time.hpp
        simNode["start_timestamp"] = fmt::format("{:%Y-%m-%dT%H:%M:%S}", config.simulation.startTimestamp);
        simNode["time_step"] = fmt::format("{:%H:%M:%S}", config.simulation.timeStep);
        simNode["duration"] = fmt::format("{:%H:%M:%S}", config.simulation.duration);

        YAML::Node viewBoxNode;
        viewBoxNode["use_full_grid"] = config.simulation.viewBox.useFullGrid;

        YAML::Node swNode;
        swNode["lon"] = config.simulation.viewBox.southWest.lon;
        swNode["lat"] = config.simulation.viewBox.southWest.lat;
        viewBoxNode["south_west"] = swNode;

        YAML::Node neNode;
        neNode["lon"] = config.simulation.viewBox.northEast.lon;
        neNode["lat"] = config.simulation.viewBox.northEast.lat;
        viewBoxNode["north_east"] = neNode;

        simNode["view_box"] = viewBoxNode;
        root["simulation"] = simNode;

        // ---------------------------------------------------------
        // 4. State Updater Config
        // ---------------------------------------------------------
        YAML::Node stateUpdaterNode;
        stateUpdaterNode["enable_rainfall"] = config.stateUpdater.enableRainfall;
        stateUpdaterNode["dry_tolerance"] = config.stateUpdater.dryTolerance;

        // Construir la estructura de risk levels según el formato original
        YAML::Node floodRiskNode;

        // El primero es el default
        const auto& defLevel = config.stateUpdater.floodRiskLevels[0];
        YAML::Node defaultLevelNode;
        defaultLevelNode["name"] = defLevel.name;
        defaultLevelNode["color_hex"] = defLevel.colorHex;
        floodRiskNode["default_level"] = defaultLevelNode;

        // El resto van al array "levels"
        YAML::Node levelsNode;
        for (size_t i = 1; i < config.stateUpdater.floodRiskLevels.size(); ++i) {
            const auto& lvl = config.stateUpdater.floodRiskLevels[i];
            YAML::Node lNode;
            lNode["name"] = lvl.name;
            lNode["threshold_start"] = lvl.thresholdStart;
            lNode["color_hex"] = lvl.colorHex;
            levelsNode.push_back(lNode);
        }
        floodRiskNode["levels"] = levelsNode;
        stateUpdaterNode["flood_risk"] = floodRiskNode;

        YAML::Node updaterNode;
        std::visit(overloaded{
            [&updaterNode](const OnnxUpdaterConfig& onnxCfg) {
                // El nombre del tipo que usas en el YAML para que el loader sepa qué instanciar
                updaterNode["type"] = std::string(magic_enum::enum_name(UpdaterConfigType::ONNX));

                // Propiedades específicas del modelo ONNX (ajusta los nombres de las variables a tu struct real)
                updaterNode["model_path"] = onnxCfg.modelPath.string();
                updaterNode["tensor_dim"] = onnxCfg.tensorDim;
            },
            [&updaterNode](auto&&) {
                // Fallback de seguridad por si se añade un tipo al variant pero no a la serialización
                updaterNode["type"] = "Unknown";
            }
            }, 
            config.stateUpdater.updater
        );
        stateUpdaterNode["updater"] = updaterNode;

        root["state_updater"] = stateUpdaterNode;

        // ---------------------------------------------------------
        // 5. Input Config
        // ---------------------------------------------------------
        YAML::Node inputNode;
        YAML::Node fileNode;
        fileNode["dataset_folder"] = config.input.file.datasetFolder.string();
        fileNode["dataset_name"] = config.input.file.datasetName;
        fileNode["static_format"] = std::string(magic_enum::enum_name(config.input.file.staticFormat));
        fileNode["dynamic_format"] = std::string(magic_enum::enum_name(config.input.file.dynamicFormat));
        inputNode["file"] = fileNode;

        YAML::Node scalarsNode;
        for (const auto& [key, val] : config.input.scalars) {
            scalarsNode[key] = val;
        }
        inputNode["scalars"] = scalarsNode;
        root["input"] = inputNode;

        // ---------------------------------------------------------
        // 6. Output Config
        // ---------------------------------------------------------
        YAML::Node outputNode;
        // Asumiendo que config.output.outputs es un vector de std::variant
        
        YAML::Node snapshotNode;
        snapshotNode["every_n_steps"] = config.output.snapshot.everyNSteps;
        outputNode["snapshot"] = snapshotNode;

        YAML::Node outputsList;
        for (const auto& outVar : config.output.outputs) {
            YAML::Node outItem;
            std::visit(overloaded{
                [&outItem](const OutputConfig::CheckpointOutputConfigEntry& checkpoint) {
                    outItem["type"] = std::string(magic_enum::enum_name(OutputConfig::OutputConfigEntryType::CHECKPOINT));
                    outItem["static_format"] = std::string(magic_enum::enum_name(checkpoint.staticFormat));
                },
                [&outItem](const OutputConfig::ImageOutputConfigEntry& img) {
                    outItem["type"] = std::string(magic_enum::enum_name(OutputConfig::OutputConfigEntryType::IMAGE));
                },
                [&outItem](const OutputConfig::MqttOutputConfigEntry& mqtt) {
                    outItem["type"] = std::string(magic_enum::enum_name(OutputConfig::OutputConfigEntryType::MQTT));
                    outItem["protocol"] = mqtt.protocol;
                    outItem["host"] = mqtt.host;
                    outItem["port"] = mqtt.port;
                    outItem["payloadFormat"] = std::string(magic_enum::enum_name(mqtt.payloadFormat));
                },
                [&outItem](auto&&) {
                    // Fallback
                }
            }, outVar);
            outputsList.push_back(outItem);
        }
        outputNode["outputs"] = outputsList;
        root["output"] = outputNode;
        

        // ---------------------------------------------------------
        // ESCRITURA A ARCHIVO
        // ---------------------------------------------------------
        std::ofstream fout(destination);
        if (!fout.is_open()) {
            throw std::runtime_error(fmt::format("Failed to open file for writing: {}", destination.string()));
        }

        // yaml-cpp sobrecarga el operador << para emitir la estructura bonita
        fout << root;
        fout.close();
    }

} // namespace danasim
