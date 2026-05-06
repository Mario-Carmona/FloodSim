
#pragma once

#include <imgui.h>
#include <atomic>
#include <memory>
#include <thread>
#include <algorithm>

#include "app/config/Config.hpp"

#include "gui/helpers/Helpers.hpp"

namespace danasim { class Application; }

namespace FloodSim::Gui {

	void renderMainControlTab(
		const danasim::Config& config,
		std::atomic<bool>& isSimulationRunning,
		std::shared_ptr<danasim::Application>& currentSimulation,
		std::jthread& simulationThread
	);
	void renderScenarioTab(danasim::ScenarioConfig& scenarioConfig);
	void renderLoggingTab(danasim::LoggingConfig& loggingConfig);
	void renderInputTab(danasim::InputConfig& inputConfig, const danasim::UpdaterConfig& updaterConfig);
	void renderOutputTab(danasim::OutputConfig& outputConfig);
	void renderStateUpdaterTab(danasim::StateUpdaterConfig& stateUpdaterConfig);
	void renderSimulationTab(danasim::SimulationConfig& simulationConfig);

}
