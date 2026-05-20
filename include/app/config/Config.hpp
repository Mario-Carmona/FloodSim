/**
 * @file Config.hpp
 * @brief Defines the data structures that hold the application configuration.
 *
 * This file uses std::variant to handle polymorphic configuration sections
 * and std::filesystem for robust path handling.
 *
 * @version 1.0
 * @date 2026
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <variant>
#include <unordered_map>
#include <chrono>

#include "Types.hpp"
#include "logging/LoggerLevel.hpp"
#include "app/io/formats/file/FileStaticFormat.hpp"
#include "app/io/formats/file/FileDynamicFormat.hpp"


namespace danasim {

    // -------------------------------------------------------------------------
    // Input Configuration
    // -------------------------------------------------------------------------

    struct InputConfig {
        struct FileInputSourceConfig {
            FileStaticFormat staticFormat;
            FileDynamicFormat dynamicFormat;
            std::filesystem::path datasetFolder;
            std::string datasetName; 
        };


        FileInputSourceConfig file;
        std::unordered_map<std::string, std::string> scalars;
    };

    // -------------------------------------------------------------------------
    // Output Configuration
    // -------------------------------------------------------------------------

    /**
     * @brief Container for output configuration logic.
     */
    struct OutputConfig {

        enum class OutputConfigEntryType {
            MQTT,
            IMAGE,
            CHECKPOINT
        };

        struct CheckpointOutputConfigEntry {
            FileStaticFormat staticFormat;
        };

        struct MqttOutputConfigEntry {

            enum class PayloadFormat {
                JSON
            };

            std::string protocol = "mqtt://";
            std::string host = "127.0.0.1";
            int port = 1883;
            PayloadFormat payloadFormat;
        };

        struct ImageOutputConfigEntry {
            // Placeholder for future image config
        };

        /// Variant for polymorphic output types.
        using OutputConfigEntry = std::variant<CheckpointOutputConfigEntry, MqttOutputConfigEntry, ImageOutputConfigEntry>;

        /**
         * @struct SnapshotConfig
         * @brief Controls how often the simulation state is captured.
         */
        struct SnapshotConfig {
            StepType everyNSteps = 1; ///< Default: Capture every step.
        };

        std::vector<OutputConfigEntry> outputs;
        SnapshotConfig snapshot;
    };

    // -------------------------------------------------------------------------
    // State Updater Configuration
    // -------------------------------------------------------------------------

    enum class UpdaterConfigType {
        ONNX
    };

    struct OnnxUpdaterConfig {
        std::filesystem::path modelPath;
        int64_t tensorDim;
    };

    using UpdaterConfig = std::variant<OnnxUpdaterConfig>;

    struct FloodRiskLevel {
        std::string name;
        float thresholdStart;
        std::string colorHex;
    };

    struct StateUpdaterConfig {
        UpdaterConfig updater;
        bool enableRainfall;
        float dryTolerance;
        std::vector<FloodRiskLevel> floodRiskLevels;
    };

    // -------------------------------------------------------------------------
    // General Settings
    // -------------------------------------------------------------------------

    struct ViewBox {

        struct Point {
            double lon;
            double lat;
        };
        
        bool useFullGrid;
        Point southWest;
        Point northEast;
    };

    struct SimulationConfig {
        std::chrono::sys_seconds startTimestamp;
        std::chrono::seconds timeStep;    ///< Simulation step delta in seconds.
        std::chrono::seconds duration;   ///< Total duration in seconds.
        ViewBox viewBox;
    };

    struct ScenarioConfig {
        std::filesystem::path outputDir;
        std::string name;
        bool appendStartTimestamp;
    };

    struct LoggingConfig {
        LoggerLevel level;
        bool async;
        bool silent;
        bool saveLogFile;
    };

    /**
     * @struct Config
     * @brief Root configuration object aggregating all modules.
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
