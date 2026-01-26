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

#include "Types.hpp"

namespace danasim {

    // -------------------------------------------------------------------------
    // Input Configuration
    // -------------------------------------------------------------------------

    /**
     * @enum InputConfigType
     * @brief Supported input source types.
     */
    enum class InputConfigType {
        FILE
    };

    /**
     * @struct FileInputConfig
     * @brief Configuration for file-based inputs.
     */
    struct FileInputConfig {
        std::filesystem::path path; ///< Path to the input file.
    };

    /// Variant holding the active input configuration.
    using InputConfig = std::variant<FileInputConfig>;

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

        struct X3DFileOutputConfigEntry {
            std::filesystem::path filePath;
        };

        struct MQTTOutputConfigEntry {

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
        using OutputConfigEntry = std::variant<X3DFileOutputConfigEntry, MQTTOutputConfigEntry, ImageOutputConfigEntry>;

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
        TENSORFLOW
    };

    struct TensorFlowStateUpdaterConfig {
        std::filesystem::path modelPath;
    };

    using StateUpdaterConfig = std::variant<TensorFlowStateUpdaterConfig>;

    // -------------------------------------------------------------------------
    // General Settings
    // -------------------------------------------------------------------------

    struct SimulationConfig {
        float timeStep = 0.1f;    ///< Simulation step delta in seconds.
        float totalTime = 0.0f;   ///< Total duration in seconds.
        float viewMinX; 
        float viewMaxX;
        float viewMinY;
        float viewMaxY;
    };

    struct ScenarioConfig {
        std::string name;
    };

    struct LoggingConfig {
        std::string level = "info";
        bool async = false;
        bool silent = false;
        std::filesystem::path file;
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
