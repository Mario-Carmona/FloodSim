
"""
config.py
Central configuration file for the Simulation Monitor.
"""

from datetime import datetime, timezone
import os
from pathlib import Path

# Simulation constraints
MAP_SIZE = 9403

PROJECT_ROOT = Path(__file__).resolve().parents[2]
COLOR_PALETTE_FILE = Path(
	os.getenv("DANASIM_COLOR_PALETTE", str(PROJECT_ROOT / "data_29_10_2024" / "color_palette.json"))
)
COLOR_PALETTE_LAYER = os.getenv("DANASIM_COLOR_LAYER", "flood_risk")
DEFAULT_DATA_ROOT = Path(os.getenv("DANASIM_DATA_ROOT", str(PROJECT_ROOT)))
PALETTE_MAX_VALUE = int(os.getenv("DANASIM_PALETTE_MAX", "5"))
RENDER_ON_INIT_EOF = os.getenv("DANASIM_RENDER_ON_INIT_EOF", "1") == "1"

# Fallback mapping when events provide textual state names instead of numeric values.
STATE_TO_VALUE = {
	"DRY": 0,
	"VERY_SHALLOW": 1,
	"LOW_DEPTH": 2,
	"MEDIUM_DEPTH": 3,
	"FLOODED": 3,
	"HIGH_DEPTH": 4,
	"EXTREME_DEPTH": 5,
	"OBSTACLE_DESTROYED": 0,
	"OPEN": 0,
}

# MQTT Broker Configuration
BROKER_ADDRESS = os.getenv("DANASIM_MQTT_HOST", "localhost")
BROKER_PORT = int(os.getenv("DANASIM_MQTT_PORT", "1883"))
CLIENT_ID = os.getenv("DANASIM_MQTT_CLIENT_ID", "Danasim_Monitor_Viewer")

# Scenario and topic structure
SCENARIO_NAME = os.getenv("DANASIM_SCENARIO", "scenario_local")
TOPIC_BASE = f"FloodSim/{SCENARIO_NAME}"
TOPIC_EVENTS = f"{TOPIC_BASE}/events"
TOPIC_SYSTEM = f"{TOPIC_BASE}/system"
TOPIC_HANDSHAKE_PING = f"{TOPIC_SYSTEM}/handshake/ping"
TOPIC_HANDSHAKE_PONG = f"{TOPIC_SYSTEM}/handshake/pong"
TOPIC_CONTROL_EVENTS = f"{TOPIC_BASE}/control/events"

# MQTT delivery settings
QOS_HANDSHAKE = 1
QOS_EVENTS = int(os.getenv("DANASIM_QOS_EVENTS", "1"))
KEEPALIVE_SECONDS = 60

# Handshake defaults
HANDSHAKE_TIMEOUT_SECONDS = float(os.getenv("DANASIM_HANDSHAKE_TIMEOUT", "5"))
HANDSHAKE_MAX_RETRIES = int(os.getenv("DANASIM_HANDSHAKE_MAX_RETRIES", "3"))


def utc_now_iso() -> str:
	return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")

# Visualization settings
REFRESH_RATE_SECONDS = float(os.getenv("DANASIM_REFRESH_RATE_SECONDS", "1000000000"))  # Interval for UI updates
IDLE_SLEEP_SECONDS = 0.1    # CPU saver when no data is arriving
DEBUG_MODE = True
