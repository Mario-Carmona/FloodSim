/**
 * @file Config.hpp
 * @brief Defines the data structures that hold the application configuration.
 *
 * This file uses std::variant to handle polymorphic configuration sections
 * and std::filesystem for robust path handling.
 *
 * @version 1.0
 * @date 2026
 * @copyright Copyright (c) 2026 Danasim
 */

#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <variant>
#include <unordered_map>
#include <chrono>

#include "Types.hpp"

namespace danasim {

    // -------------------------------------------------------------------------
    // Input Configuration
    // -------------------------------------------------------------------------

    struct InputConfig {
        enum class InputSourceConfigEntryType {
            FILE
        };

        struct FileInputSourceConfigEntry {
            std::string staticFormat;
            std::string dynamicFormat;
            std::filesystem::path path; ///< Path to the input file.
        };

        using InputSourceConfigEntry = std::variant<FileInputSourceConfigEntry>;


        struct InputLayerConfigEntry {
            std::string name;
            std::string source;
        };


        std::unordered_map<std::string, InputSourceConfigEntry> sources;
        std::vector<InputLayerConfigEntry> layers;
    };

    // -------------------------------------------------------------------------
    // Output Configuration
    // -------------------------------------------------------------------------

    /**
     * @brief Container for output configuration logic.
     */
    struct OutputConfig {

        enum class OutputConfigEntryType {
            X3D_FILE,
            MQTT,
            IMAGE
        };

        struct X3dFileOutputConfigEntry {
            // Placeholder for future X3D config
        };

        struct MqttOutputConfigEntry {

            enum class PayloadFormat {
                PROTOBUF
            };

            std::string address;
            std::string topic;
            std::string clientId;
            int qos;
            PayloadFormat payloadFormat;
        };

        struct ImageOutputConfigEntry {
            // Placeholder for future image config
        };

        /// Variant for polymorphic output types.
        using OutputConfigEntry = std::variant<X3dFileOutputConfigEntry, MqttOutputConfigEntry, ImageOutputConfigEntry>;

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

    enum class StateUpdaterConfigType {
        ONNX
    };

    struct OnnxStateUpdaterConfig {
        std::filesystem::path modelPath;
    };

    using StateUpdaterConfig = std::variant<OnnxStateUpdaterConfig>;

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
    };

    struct LoggingConfig {
        std::string level = "info";
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
