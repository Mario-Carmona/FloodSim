
#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <variant>

namespace danasim {

    /**
     * @brief Input configuration
     */
    enum class InputConfigType {
        FILE,
        MQTT
    };

    // Configuración específica para archivos GIS
    struct FileInputConfig {
        std::string path;
    };

    // Configuración específica para tiempo real vía MQTT
    struct MQTTInputConfig {
        std::string broker;
        std::string topic;
    };

    using InputConfig = std::variant<FileInputConfig, MQTTInputConfig>;

    /**
     * @brief Output configuration
     */
    struct OutputConfig {
        enum class OutputConfigEntryType {
            X3D_FILE,
            MQTT
        };

        enum class MQTTFormat {
            PROTOBUF
        };

        struct X3DFileOutputConfigEntry {
            std::filesystem::path filePath;
        };

        struct MQTTOutputConfigEntry {
            std::string address;
            std::string topic;
            std::string clientId;
            int qos;
            MQTTFormat format;
        };

        using OutputConfigEntry = std::variant<X3DFileOutputConfigEntry, MQTTOutputConfigEntry>;

        struct SnapshotConfig {
            std::size_t everyNSteps;
        };

        std::vector<OutputConfigEntry> outputs;
        SnapshotConfig snapshot;
    };


    /**
     * @brief State updater configuration
	 */
    enum class StateUpdaterConfigType {
        TENSORFLOW,
        EXPERIMENTAL
    };

    struct TensorFlowStateUpdaterConfig {
        std::string modelPath;
    };

    struct ExperimentalStateUpdaterConfig {};

    using StateUpdaterConfig = std::variant<TensorFlowStateUpdaterConfig, ExperimentalStateUpdaterConfig>;


    /**
     * @brief Simulation (core) configuration
     */
    struct SimulationConfig {
        float timeStep;     // seconds
        float totalTime;    // seconds
    };


    struct ScenarioConfig {
        std::string name;
    };


    struct LoggingConfig {
        std::string level;
        bool async;
        bool silent;
        std::string file;
    };


    /**
     * @brief Root configuration object
     */
    struct Config {
        LoggingConfig logging;
        InputConfig input;
        OutputConfig output;
        StateUpdaterConfig stateUpdater;
        SimulationConfig simulation;
        ScenarioConfig scenario;
    };

} // namespace danasim
