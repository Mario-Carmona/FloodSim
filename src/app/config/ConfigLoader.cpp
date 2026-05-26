/**
 * @file ConfigLoader.cpp
 * @brief Implementation of the YAML configuration loader, including exception handling.
 *
 * @copyright Copyright (c) 2026 FloodSim
 */

#include "app/config/ConfigLoader.hpp"

#include <algorithm>
#include <yaml-cpp/yaml.h>
#include <string_view>
#include <filesystem>
#include <fmt/core.h>
#include <magic_enum/magic_enum.hpp>
#include <whereami.h>
#include <fmt/chrono.h>
#include <fstream>

#include "logging/LoggerLevel.hpp"
#include "misc/TimeUtils.hpp" 

namespace floodsim::app::config {

namespace {

/**
 * @brief Retrieves the standard operating system directory for storing application data.
 *
 * Resolves the appropriate user-specific data folder based on the OS (Windows APPDATA,
 * macOS Library/Application Support, or Linux XDG Base Directory specification).
 *
 * @param app_name The name of your application to create a dedicated subfolder.
 * @return std::filesystem::path The absolute path to the application's data directory.
 */
[[nodiscard]] inline std::filesystem::path GetAppDataDirectory(std::string_view app_name) {
    std::filesystem::path app_data_path;

#if defined(_WIN32)
    // --- WINDOWS ---
    const char* app_data = std::getenv("APPDATA");
    if (app_data != nullptr) {
        app_data_path = std::filesystem::path(app_data);
    }
    else {
        // Unlikely fallback on Windows, but safe
        app_data_path = std::filesystem::current_path();
    }
#elif defined(__APPLE__)
    // --- macOS ---
    const char* home = std::getenv("HOME");
    if (home != nullptr) {
        app_data_path = std::filesystem::path(home) / "Library" / "Application Support";
    }
#elif defined(__linux__) || defined(__unix__)
    // --- LINUX / UNIX ---
    const char* xdg_data_home = std::getenv("XDG_DATA_HOME");
    if (xdg_data_home != nullptr && xdg_data_home[0] != '\0') {
        app_data_path = std::filesystem::path(xdg_data_home);
    }
    else {
        // Standard XDG fallback if the environment variable is not defined
        const char* home = std::getenv("HOME");
        if (home != nullptr) {
            app_data_path = std::filesystem::path(home) / ".local" / "share";
        }
    }
#else
#error "Operating system not supported"
#endif

    // If we successfully obtained a valid base path, append our app's folder
    if (!app_data_path.empty() && !app_name.empty()) {
        app_data_path /= app_name;
    }

    return app_data_path;
}

/**
 * @class ConfigurationException
 * @brief Exception thrown when a configuration error occurs (parsing, validation, or I/O).
 */
class ConfigurationException : public std::runtime_error {
public:
    explicit ConfigurationException(const std::string& message)
        : std::runtime_error("Configuration Error: " + message) {}
};

/**
 * @brief Checks if a YAML node exists, throws descriptive error if not.
 */
YAML::Node RequireNode(const YAML::Node& parent, const std::string& key) {
    if (!parent[key]) {
        throw ConfigurationException("Missing required configuration section: '" + key + "'");
    }
    return parent[key];
}

/**
* @brief Safely extracts a value from a node with type checking.
*/
template <typename T>
T Extract(const YAML::Node& node, const std::string& key) {
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

/**
 * @brief Safely extracts an enum from a string, handling Google C++ Style "k" prefixes automatically.
 */
template <typename EnumType>
EnumType ExtractEnum(const YAML::Node& node, const std::string& key) {
    std::string raw_value = Extract<std::string>(node, key);

    std::string k_prefixed = "k" + raw_value;
    auto enum_opt = magic_enum::enum_cast<EnumType>(k_prefixed, magic_enum::case_insensitive);
    if (enum_opt.has_value()) {
        return enum_opt.value();
    }

    throw ConfigurationException(fmt::format("Field '{}' contains invalid enum value: '{}'", key, raw_value));
}

void Validate(bool condition, const std::string& message) {
    if (!condition) {
        throw ConfigurationException(message);
    }
}

// Helper para std::visit
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

} // anonymous namespace

Config ConfigLoader::Load(const std::filesystem::path& config_path) {
    if (!std::filesystem::exists(config_path)) {
        throw ConfigurationException("Configuration file not found: " + config_path.string());
    }

    YAML::Node root;
    try {
        root = YAML::LoadFile(config_path.string());
    }
    catch (const YAML::ParserException& e) {
        throw ConfigurationException("YAML Syntax Error: " + std::string(e.what()));
    }
    catch (const std::exception& e) {
        throw ConfigurationException("Unexpected error loading file: " + std::string(e.what()));
    }

    const std::filesystem::path config_folder = config_path.parent_path();
    Config config;

    try {
        // -----------------------------
        // INPUT
        // -----------------------------
        {
            const auto node = RequireNode(root, "input");

            {
                const auto file_node = RequireNode(node, "file");

                config.input.file = InputConfig::FileInputSourceConfig{
                  .static_format = ExtractEnum<io::formats::file::FileStaticFormat>(file_node, "static_format"),
                  .dynamic_format = ExtractEnum<io::formats::file::FileDynamicFormat>(file_node, "dynamic_format"),
                  .dataset_folder = (config_folder / Extract<std::string>(file_node, "dataset_folder")).lexically_normal(),
                  .dataset_name = Extract<std::string>(file_node, "dataset_name")
                };
            }

            {
                const auto scalars_node = RequireNode(node, "scalars");

                for (const auto& it : scalars_node) {
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
            const auto node = RequireNode(root, "output");

            const auto snapshot_node = RequireNode(node, "snapshot");
            config.output.snapshot.every_n_steps = Extract<std::size_t>(snapshot_node, "every_n_steps");

            Validate(config.output.snapshot.every_n_steps > 0, "snapshot.every_n_steps must be > 0");

            const auto outputs_node = RequireNode(node, "outputs");

            for (const auto& out_node : outputs_node) {
                OutputConfig::OutputConfigEntryType type = ExtractEnum<OutputConfig::OutputConfigEntryType>(out_node, "type");

                switch (type) {
                case OutputConfig::OutputConfigEntryType::kCheckpoint: {
                    config.output.outputs.emplace_back(OutputConfig::CheckpointOutputConfigEntry{
                      .static_format = ExtractEnum<io::formats::file::FileStaticFormat>(out_node, "static_format")
                        });
                    break;
                }
                case OutputConfig::OutputConfigEntryType::kMqtt: {
                    config.output.outputs.emplace_back(OutputConfig::MqttOutputConfigEntry{
                      .protocol = Extract<std::string>(out_node, "protocol"),
                      .host = Extract<std::string>(out_node, "host"),
                      .port = Extract<int>(out_node, "port"),
                      .payload_format = ExtractEnum<OutputConfig::MqttOutputConfigEntry::PayloadFormat>(out_node, "format")
                        });
                    break;
                }
                case OutputConfig::OutputConfigEntryType::kImage: {
                    config.output.outputs.emplace_back(OutputConfig::ImageOutputConfigEntry{});
                    break;
                }
                }
            }

            Validate(!config.output.outputs.empty(), "At least one output must be defined");
        }

        // -----------------------------
        // SIMULATION
        // -----------------------------
        {
            const auto node = RequireNode(root, "simulation");

            config.simulation.start_timestamp = ParseTimestampString(Extract<std::string>(node, "start_timestamp"));
            config.simulation.time_step = ParseDurationString(Extract<std::string>(node, "time_step"));
            config.simulation.duration = ParseDurationString(Extract<std::string>(node, "duration"));

            {
                const auto view_node = RequireNode(node, "view_box");

                config.simulation.view_box.use_full_grid = Extract<bool>(view_node, "use_full_grid");

                if (!config.simulation.view_box.use_full_grid) {
                    {
                        const auto south_west_node = RequireNode(view_node, "south_west");
                        config.simulation.view_box.south_west.lon = Extract<double>(south_west_node, "lon");
                        config.simulation.view_box.south_west.lat = Extract<double>(south_west_node, "lat");
                    }

                    {
                        const auto north_east_node = RequireNode(view_node, "north_east");
                        config.simulation.view_box.north_east.lon = Extract<double>(north_east_node, "lon");
                        config.simulation.view_box.north_east.lat = Extract<double>(north_east_node, "lat");
                    }
                }
            }

            Validate(config.simulation.time_step > config.simulation.time_step.zero(), "simulation.time_step must be > 0s");
            Validate(config.simulation.duration > config.simulation.duration.zero(), "simulation.duration must be > 0s");
        }

        // -----------------------------
        // LOGGING
        // -----------------------------
        {
            const auto node = RequireNode(root, "logging");

            config.logging.level = ExtractEnum<logging::LoggerLevel>(node, "level");
            config.logging.async = Extract<bool>(node, "async");
            config.logging.silent = Extract<bool>(node, "silent");
            config.logging.save_log_file = Extract<bool>(node, "save_log_file");
        }

        // -----------------------------
        // SCENARIO
        // -----------------------------
        {
            const auto node = RequireNode(root, "scenario");

            if (node["output_dir"]) {
                config.scenario.output_dir = (config_folder / Extract<std::string>(node, "output_dir")).lexically_normal();
            }
            else {
                config.scenario.output_dir = GetAppDataDirectory(FLOODSIM_PROGRAM_NAME);
            }

            config.scenario.name = Extract<std::string>(node, "name");
            config.scenario.append_start_timestamp = Extract<bool>(node, "append_start_timestamp");
        }

        // -----------------------------
        // STATE UPDATER
        // -----------------------------
        {
            const auto node = RequireNode(root, "state_updater");

            {
                const auto updater_node = RequireNode(node, "updater");

                UpdaterConfigType type = ExtractEnum<UpdaterConfigType>(updater_node, "type");

                switch (type) {
                case UpdaterConfigType::kOnnx: {
                    config.state_updater.updater = OnnxUpdaterConfig{
                      .model_path = (config_folder / Extract<std::string>(updater_node, "model_path")).lexically_normal(),
                      .tensor_dim = Extract<int64_t>(updater_node, "tensor_dim")
                    };
                    break;
                }
                }
            }

            config.state_updater.enable_rainfall = Extract<bool>(node, "enable_rainfall");
            config.state_updater.dry_tolerance = Extract<float>(node, "dry_tolerance");

            {
                const auto flood_risk_node = RequireNode(node, "flood_risk");
                const auto default_level_node = RequireNode(flood_risk_node, "default_level");

                FloodRiskLevel default_level{
                  .name = Extract<std::string>(default_level_node, "name"),
                  .threshold_start = 0.0f,
                  .color_hex = Extract<std::string>(default_level_node, "color_hex")
                };

                config.state_updater.flood_risk_levels.emplace_back(default_level);

                const auto levels_node = RequireNode(flood_risk_node, "levels");

                for (const auto& elem_node : levels_node) {
                    config.state_updater.flood_risk_levels.emplace_back(FloodRiskLevel{
                      .name = Extract<std::string>(elem_node, "name"),
                      .threshold_start = Extract<float>(elem_node, "threshold_start"),
                      .color_hex = Extract<std::string>(elem_node, "color_hex")
                        });
                }

                Validate(!config.state_updater.flood_risk_levels.empty(), "At least one flood risk level must be defined");

                std::sort(
                    config.state_updater.flood_risk_levels.begin(),
                    config.state_updater.flood_risk_levels.end(),
                    [](const FloodRiskLevel& a, const FloodRiskLevel& b) {
                        return a.threshold_start < b.threshold_start;
                    }
                );

                Validate(config.state_updater.flood_risk_levels[0].name == default_level.name, "First flood risk level must be the default level");
            }
        }

    }
    catch (const ConfigurationException&) {
        throw; // Rethrow custom exceptions
    }
    catch (const std::exception& e) {
        throw ConfigurationException("Error parsing configuration structure: " + std::string(e.what()));
    }

    return config;
}

void ConfigLoader::SaveToFile(const std::filesystem::path& destination, const Config& config) {
    try {
        YAML::Node root;

        // ---------------------------------------------------------
        // 1. Logging Config
        // ---------------------------------------------------------
        YAML::Node logging_node;
        logging_node["level"] = std::string(magic_enum::enum_name(config.logging.level));
        logging_node["async"] = config.logging.async;
        logging_node["silent"] = config.logging.silent;
        logging_node["save_log_file"] = config.logging.save_log_file;
        root["logging"] = logging_node;

        // ---------------------------------------------------------
        // 2. Scenario Config
        // ---------------------------------------------------------
        YAML::Node scenario_node;
        scenario_node["name"] = config.scenario.name;
        scenario_node["output_dir"] = config.scenario.output_dir.string();
        scenario_node["append_start_timestamp"] = config.scenario.append_start_timestamp;
        root["scenario"] = scenario_node;

        // ---------------------------------------------------------
        // 3. Simulation Config
        // ---------------------------------------------------------
        YAML::Node sim_node;
        sim_node["start_timestamp"] = fmt::format("{:%Y-%m-%dT%H:%M:%S}", config.simulation.start_timestamp);
        sim_node["time_step"] = fmt::format("{:%H:%M:%S}", config.simulation.time_step);
        sim_node["duration"] = fmt::format("{:%H:%M:%S}", config.simulation.duration);

        YAML::Node view_box_node;
        view_box_node["use_full_grid"] = config.simulation.view_box.use_full_grid;

        YAML::Node sw_node;
        sw_node["lon"] = config.simulation.view_box.south_west.lon;
        sw_node["lat"] = config.simulation.view_box.south_west.lat;
        view_box_node["south_west"] = sw_node;

        YAML::Node ne_node;
        ne_node["lon"] = config.simulation.view_box.north_east.lon;
        ne_node["lat"] = config.simulation.view_box.north_east.lat;
        view_box_node["north_east"] = ne_node;

        sim_node["view_box"] = view_box_node;
        root["simulation"] = sim_node;

        // ---------------------------------------------------------
        // 4. State Updater Config
        // ---------------------------------------------------------
        YAML::Node state_updater_node;
        state_updater_node["enable_rainfall"] = config.state_updater.enable_rainfall;
        state_updater_node["dry_tolerance"] = config.state_updater.dry_tolerance;

        YAML::Node flood_risk_node;

        const auto& def_level = config.state_updater.flood_risk_levels[0];
        YAML::Node default_level_node;
        default_level_node["name"] = def_level.name;
        default_level_node["color_hex"] = def_level.color_hex;
        flood_risk_node["default_level"] = default_level_node;

        YAML::Node levels_node;
        for (size_t i = 1; i < config.state_updater.flood_risk_levels.size(); ++i) {
            const auto& lvl = config.state_updater.flood_risk_levels[i];
            YAML::Node l_node;
            l_node["name"] = lvl.name;
            l_node["threshold_start"] = lvl.threshold_start;
            l_node["color_hex"] = lvl.color_hex;
            levels_node.push_back(l_node);
        }
        flood_risk_node["levels"] = levels_node;
        state_updater_node["flood_risk"] = flood_risk_node;

        YAML::Node updater_node;
        std::visit(overloaded{
          [&updater_node](const OnnxUpdaterConfig& onnx_cfg) {
            updater_node["type"] = std::string(magic_enum::enum_name(UpdaterConfigType::kOnnx));
            updater_node["model_path"] = onnx_cfg.model_path.string();
            updater_node["tensor_dim"] = onnx_cfg.tensor_dim;
          },
          [&updater_node](auto&&) {
            updater_node["type"] = "Unknown";
          }
            }, config.state_updater.updater);

        state_updater_node["updater"] = updater_node;
        root["state_updater"] = state_updater_node;

        // ---------------------------------------------------------
        // 5. Input Config
        // ---------------------------------------------------------
        YAML::Node input_node;
        YAML::Node file_node;
        file_node["dataset_folder"] = config.input.file.dataset_folder.string();
        file_node["dataset_name"] = config.input.file.dataset_name;
        file_node["static_format"] = std::string(magic_enum::enum_name(config.input.file.static_format));
        file_node["dynamic_format"] = std::string(magic_enum::enum_name(config.input.file.dynamic_format));
        input_node["file"] = file_node;

        YAML::Node scalars_node;
        for (const auto& [key, val] : config.input.scalars) {
            scalars_node[key] = val;
        }
        input_node["scalars"] = scalars_node;
        root["input"] = input_node;

        // ---------------------------------------------------------
        // 6. Output Config
        // ---------------------------------------------------------
        YAML::Node output_node;

        YAML::Node snapshot_node;
        snapshot_node["every_n_steps"] = config.output.snapshot.every_n_steps;
        output_node["snapshot"] = snapshot_node;

        YAML::Node outputs_list;
        for (const auto& out_var : config.output.outputs) {
            YAML::Node out_item;
            std::visit(overloaded{
              [&out_item](const OutputConfig::CheckpointOutputConfigEntry& checkpoint) {
                out_item["type"] = std::string(magic_enum::enum_name(OutputConfig::OutputConfigEntryType::kCheckpoint));
                out_item["static_format"] = std::string(magic_enum::enum_name(checkpoint.static_format));
              },
              [&out_item](const OutputConfig::ImageOutputConfigEntry& img) {
                out_item["type"] = std::string(magic_enum::enum_name(OutputConfig::OutputConfigEntryType::kImage));
              },
              [&out_item](const OutputConfig::MqttOutputConfigEntry& mqtt) {
                out_item["type"] = std::string(magic_enum::enum_name(OutputConfig::OutputConfigEntryType::kMqtt));
                out_item["protocol"] = mqtt.protocol;
                out_item["host"] = mqtt.host;
                out_item["port"] = mqtt.port;
                out_item["format"] = std::string(magic_enum::enum_name(mqtt.payload_format));
              },
              [&out_item](auto&&) {
                    // Fallback
                  }
                }, out_var);
            outputs_list.push_back(out_item);
        }
        output_node["outputs"] = outputs_list;
        root["output"] = output_node;

        // ---------------------------------------------------------
        // ESCRITURA A ARCHIVO (CON CAPTURA DE EXCEPCIONES)
        // ---------------------------------------------------------

        // Configuramos los flags para que fstream lance excepciones si algo falla gravemente
        std::ofstream fout;
        fout.exceptions(std::ofstream::failbit | std::ofstream::badbit);

        try {
            fout.open(destination);
            fout << root;
            fout.close();
        }
        catch (const std::ofstream::failure& e) {
            throw ConfigurationException(fmt::format("Failed to write to file '{}': {}", destination.string(), e.what()));
        }

    }
    catch (const ConfigurationException&) {
        throw; // Rethrow direct configuration exceptions
    }
    catch (const YAML::Exception& e) {
        throw ConfigurationException("YAML Error while saving config: " + std::string(e.what()));
    }
    catch (const std::exception& e) {
        throw ConfigurationException("Unexpected error while saving config: " + std::string(e.what()));
    }
}

} // namespace floodsim::app::config
