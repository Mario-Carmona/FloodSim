/**
 * \file Tabs.hpp
 * \brief Declarations for the GUI configuration tabs.
 *
 * This header defines the rendering functions for various tabs
 * used in the Flood Simulator GUI.
 */

#ifndef FLOODSIM_GUI_TABS_HPP_
#define FLOODSIM_GUI_TABS_HPP_

#include <atomic>
#include <memory>
#include <thread>

#include "app/config/Config.hpp"
#include "gui/tabs/helpers/Helpers.hpp"
#include "app/Application.hpp"

namespace floodsim::gui {

    /**
        * \brief Renders the Main Control tab.
        * \param config The main application configuration.
        * \param is_simulation_running Atomic flag indicating if the simulation is active.
        * \param current_simulation Shared pointer to the active application simulation.
        * \param simulation_thread Reference to the thread managing the simulation execution.
        */
    void RenderMainControlTab(
        const danasim::Config& config,
        std::atomic<bool>& is_simulation_running,
        std::shared_ptr<danasim::Application>& current_simulation,
        std::jthread& simulation_thread
    );

    /**
        * \brief Renders the Scenario configuration tab.
        * \param scenario_config Reference to the scenario configuration data to modify.
        */
    void RenderScenarioTab(danasim::ScenarioConfig& scenario_config);

    /**
        * \brief Renders the Logging configuration tab.
        * \param logging_config Reference to the logging configuration data to modify.
        */
    void RenderLoggingTab(danasim::LoggingConfig& logging_config);

    /**
        * \brief Renders the Input data configuration tab.
        * \param input_config Reference to the input configuration data to modify.
        * \param updater_config Reference to the state updater configuration (read-only context).
        */
    void RenderInputTab(danasim::InputConfig& input_config, const danasim::UpdaterConfig& updater_config);

    /**
        * \brief Renders the Output data configuration tab.
        * \param output_config Reference to the output configuration data to modify.
        */
    void RenderOutputTab(danasim::OutputConfig& output_config);

    /**
        * \brief Renders the State Updater configuration tab.
        * \param state_updater_config Reference to the state updater configuration data to modify.
        */
    void RenderStateUpdaterTab(danasim::StateUpdaterConfig& state_updater_config);

    /**
        * \brief Renders the Simulation parameters configuration tab.
        * \param simulation_config Reference to the simulation configuration data to modify.
        */
    void RenderSimulationTab(danasim::SimulationConfig& simulation_config);

}  // namespace floodsim::gui

#endif  // FLOODSIM_GUI_TABS_HPP_
