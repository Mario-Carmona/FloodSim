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

namespace floodsim {

/**
 * @brief Holds the input configuration including dataset paths and scalar mappings.
 */
struct InputConfig {
    /**
     * @brief Configuration for file-based input sources.
     */
    struct FileInputSourceConfig {
        FileStaticFormat static_format;
        FileDynamicFormat dynamic_format;
        std::filesystem::path dataset_folder;
        std::string dataset_name;
    };
    
    FileInputSourceConfig file;
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
        FileStaticFormat static_format;
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

        std::string protocol = "mqtt://";
        std::string host = "127.0.0.1";
        int port = 1883;
        PayloadFormat payload_format;
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

    std::vector<OutputConfigEntry> outputs;
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
    std::filesystem::path model_path;
    int64_t tensor_dim;
};

/// Type alias for the polymorphic updater configuration.
using UpdaterConfig = std::variant<OnnxUpdaterConfig>;

/**
 * @brief Defines a flood risk tier, its numeric thresholds, and visual representation.
 */
struct FloodRiskLevel {
    std::string name;
    float threshold_start;
    std::string color_hex;
};

/**
 * @brief Holds parameters for updating the physical simulation state.
 */
struct StateUpdaterConfig {
    UpdaterConfig updater;
    bool enable_rainfall;
    float dry_tolerance;
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
        double lon;
        double lat;
    };

    bool use_full_grid;
    Point south_west;
    Point north_east;
};

/**
 * @brief General simulation temporal and spatial parameters.
 */
struct SimulationConfig {
    std::chrono::sys_seconds start_timestamp;
    /// Simulation step delta in seconds.
    std::chrono::seconds time_step;
    /// Total duration of the simulation in seconds.
    std::chrono::seconds duration;
    ViewBox view_box;
};

/**
 * @brief Metadata and output location settings for a specific scenario execution.
 */
struct ScenarioConfig {
    std::filesystem::path output_dir;
    std::string name;
    bool append_start_timestamp;
};

/**
 * @brief Configuration for the application's internal logging system.
 */
struct LoggingConfig {
    LoggerLevel level;
    bool async;
    bool silent;
    bool save_log_file;
};

/**
 * @brief Root configuration object aggregating all subsystem modules.
 */
struct Config {
    LoggingConfig logging;
    InputConfig input;
    OutputConfig output;
    StateUpdaterConfig state_updater;
    SimulationConfig simulation;
    ScenarioConfig scenario;
};

} // namespace floodsim
