/**
 * @file Config.hpp
 * @brief Defines the data structures that hold the application configuration.
 *
 * This file uses std::variant to handle polymorphic configuration sections
 * and std::filesystem for robust path handling.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "misc/Types.hpp"
#include "app/io/formats/file/FileDynamicFormat.hpp"
#include "app/io/formats/file/FileStaticFormat.hpp"
#include "logging/LoggerLevel.hpp"

namespace floodsim::app::config {

/**
 * @brief Holds the input configuration including dataset paths and scalar mappings.
 */
struct InputConfig {
    /**
     * @brief Configuration for file-based input sources.
     */
    struct FileInputSourceConfig {
        /** @brief The static format for handling file input. */
        io::formats::file::FileStaticFormat static_format;
        /** @brief The dynamic format for handling file input. */
        io::formats::file::FileDynamicFormat dynamic_format;
        /** @brief The folder containing the dataset. */
        std::filesystem::path dataset_folder;
        /** @brief The name of the dataset. */
        std::string dataset_name;
    };
    
    /** @brief Configuration for file-based input sources. */
    FileInputSourceConfig file;
    /** @brief Map of scalar parameters and their values. */
    std::unordered_map<std::string, std::string> scalars;
};

/**
 * @brief Container for output configuration logic and policies.
 */
struct OutputConfig {
    /**
     * @brief Defines the available output medium types.
     */
    enum class OutputConfigEntryType {
        kMqtt,
        kImage,
        kCheckpoint
    };

    /**
     * @brief Configuration specific to saving simulation state checkpoints.
     */
    struct CheckpointOutputConfigEntry {
        /** @brief The static format for handling checkpoint output. */
        io::formats::file::FileStaticFormat static_format;
    };

    /**
     * @brief Configuration for MQTT telemetry and data outputs.
     */
    struct MqttOutputConfigEntry {
        /**
         * @brief Defines the format of the payload sent via MQTT.
         */
        enum class PayloadFormat {
            kJson
        };

        /** @brief The protocol for the MQTT connection. */
        std::string protocol = "mqtt://";
        /** @brief The host for the MQTT connection. */
        std::string host = "127.0.0.1";
        /** @brief The port for the MQTT connection. */
        int port = 1883;
        /** @brief The format of the payload sent via MQTT. */
        PayloadFormat payload_format;
        /** @brief Whether to send the initial state via MQTT. */
		bool send_initial_state;
    };

    /**
     * @brief Configuration for rendering or saving image outputs.
     */
    struct ImageOutputConfigEntry {
        // Placeholder for future image config
    };

    /// Variant handling polymorphic output types.
    using OutputConfigEntry = std::variant<CheckpointOutputConfigEntry,
                                           MqttOutputConfigEntry,
                                           ImageOutputConfigEntry>;

    /**
     * @brief Controls the frequency at which the simulation state is captured.
     */
    struct SnapshotConfig {
        /// Number of steps between captures. Default is every step.
        StepType every_n_steps = 1;
    };

    /** @brief List of output configurations. */
    std::vector<OutputConfigEntry> outputs;
    /** @brief Configuration for snapshotting the simulation state. */
    SnapshotConfig snapshot;
};

/**
 * @brief Available types of state updaters for the simulation.
 */
enum class UpdaterConfigType {
    kOnnx
};

/**
 * @brief Configuration specific to the ONNX-based neural network updater.
 */
struct OnnxUpdaterConfig {
    /** @brief The path to the ONNX model file. */
    std::filesystem::path model_path;
    /** @brief The dimension of the input tensor. */
    int64_t tensor_dim;
};

/// Type alias for the polymorphic updater configuration.
using UpdaterConfig = std::variant<OnnxUpdaterConfig>;

/**
 * @brief Defines a flood risk tier, its numeric thresholds, and visual representation.
 */
struct FloodRiskLevel {
	/** @brief The name of the flood risk level (e.g., "Low", "Moderate", "High"). */
    std::string name;
    /** @brief The starting threshold for the flood risk level. */
    float threshold_start;
    /** @brief The hexadecimal color code for the flood risk level. */
    std::string color_hex;
};

/**
 * @brief Holds parameters for updating the physical simulation state.
 */
struct StateUpdaterConfig {
    /** @brief The configuration for the state updater. */
    UpdaterConfig updater;
    /** @brief Whether to enable rainfall simulation. */
    bool enable_rainfall;
    /** @brief The tolerance for dry conditions. */
    float dry_tolerance;
    /** @brief The list of flood risk levels. */
    std::vector<FloodRiskLevel> flood_risk_levels;
};

/**
 * @brief Defines the geographical bounding box for the simulation view.
 */
struct ViewBox {
    /**
     * @brief A 2D geographical coordinate.
     */
    struct Point {
        /** @brief The longitude coordinate. */
        double lon;
        /** @brief The latitude coordinate. */
        double lat;
    };

    /** @brief Whether to use the full grid for the simulation view. */
    bool use_full_grid;
    /** @brief The southwest corner of the view box. */
    Point south_west;
    /** @brief The northeast corner of the view box. */
    Point north_east;
};

/**
 * @brief General simulation temporal and spatial parameters.
 */
struct SimulationConfig {
    /** @brief The timestamp for the start of the simulation. */
    std::chrono::sys_seconds start_timestamp;
    /** @brief The delta time between simulation steps in seconds. */
    std::chrono::duration<double> time_step;
    /** @brief The total duration of the simulation in seconds. */
    std::chrono::seconds duration;
    /** @brief The geographical bounding box for the simulation view. */
    ViewBox view_box;
};

/**
 * @brief Metadata and output location settings for a specific scenario execution.
 */
struct ScenarioConfig {
    /** @brief The directory where simulation outputs will be saved. */
    std::filesystem::path output_dir;
    /** @brief The name of the simulation scenario. */
    std::string name;
    /** @brief Whether to append the start timestamp to the output directory name. */
    bool append_start_timestamp;
};

/**
 * @brief Configuration for the application's internal logging system.
 */
struct LoggingConfig {
    /** @brief The logging level for the application. */
    logging::LoggerLevel level;
    /** @brief Whether to use asynchronous logging. */
    bool async;
    /** @brief Whether to suppress log output. */
    bool silent;
    /** @brief Whether to save log output to a file. */
    bool save_log_file;
};

/**
 * @brief Root configuration object aggregating all subsystem modules.
 */
struct Config {
    /** @brief Configuration for the logging subsystem. */
    LoggingConfig logging;
    /** @brief Configuration for the input data sources and mappings. */
    InputConfig input;
    /** @brief Configuration for the output mediums and snapshot policies. */
    OutputConfig output;
    /** @brief Configuration for the physical state updater and its parameters. */
    StateUpdaterConfig state_updater;
    /** @brief General parameters governing the simulation execution and view. */
    SimulationConfig simulation;
    /** @brief Metadata and output settings specific to the simulation scenario. */
    ScenarioConfig scenario;
};

} // namespace floodsim::app::config
