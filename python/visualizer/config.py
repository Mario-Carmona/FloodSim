"""Central configuration — reads from mqtt.yml.

Environment variables still override yml values as a fallback, preserving
backwards compatibility with any existing scripts that set them.
"""
from __future__ import annotations

from datetime import datetime, timezone
from pathlib import Path

import yaml

_PROJECT_ROOT = Path(__file__).resolve().parents[2]
_CONFIG_PATH  = Path(__file__).parent / "mqtt.yml"

# ---------------------------------------------------------------------------
# Load YAML
# ---------------------------------------------------------------------------
with _CONFIG_PATH.open(encoding="utf-8") as _fh:
    _cfg = yaml.safe_load(_fh)

def _get(section: str, key: str, env_var: str, default):
    """Return yml value, overridden by env var if set."""
    import os
    val = _cfg.get(section, {}).get(key, default)
    return type(default)(os.environ[env_var]) if env_var in os.environ else val

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
MAP_SIZE           = 9403
COLOR_PALETTE_FILE = _PROJECT_ROOT / "data" / "data_29_10_2024" / "color_palette.json"
FLOOD_LEVELS_FILE  = _PROJECT_ROOT / "data" / "data_29_10_2024" / "flood_levels.json"
COLOR_PALETTE_LAYER = "flood_risk"
DEFAULT_DATA_ROOT   = _PROJECT_ROOT
PALETTE_MAX_VALUE   = 5
RENDER_ON_INIT_EOF  = True

# ---------------------------------------------------------------------------
# MQTT
# ---------------------------------------------------------------------------
BROKER_ADDRESS          = _get("mqtt", "host",                 "DANASIM_MQTT_HOST",            "localhost")
BROKER_PORT             = _get("mqtt", "port",                 "DANASIM_MQTT_PORT",            1883)
CLIENT_ID               = _get("mqtt", "client_id",           "DANASIM_MQTT_CLIENT_ID",       "Danasim_Monitor_Viewer")
SCENARIO_NAME           = _get("mqtt", "scenario",            "DANASIM_SCENARIO",             "scenario_local")
QOS_HANDSHAKE           = 1
QOS_EVENTS              = _get("mqtt", "qos_events",          "DANASIM_QOS_EVENTS",           1)
KEEPALIVE_SECONDS       = _get("mqtt", "keepalive",           "DANASIM_KEEPALIVE",            60)
HANDSHAKE_TIMEOUT_SECONDS = _get("mqtt", "handshake_timeout", "DANASIM_HANDSHAKE_TIMEOUT",    5.0)
HANDSHAKE_MAX_RETRIES   = _get("mqtt", "handshake_max_retries", "DANASIM_HANDSHAKE_MAX_RETRIES", 3)

TOPIC_BASE             = f"FloodSim/{SCENARIO_NAME}"
TOPIC_EVENTS           = f"{TOPIC_BASE}/events"
TOPIC_SYSTEM           = f"{TOPIC_BASE}/system"
TOPIC_HANDSHAKE_PING   = f"{TOPIC_SYSTEM}/handshake/ping"
TOPIC_HANDSHAKE_PONG   = f"{TOPIC_SYSTEM}/handshake/pong"
TOPIC_CONTROL_EVENTS   = f"{TOPIC_BASE}/control/events"

# ---------------------------------------------------------------------------
# Renderer
# ---------------------------------------------------------------------------
RENDERER_TYPE        = _get("renderer", "type",           "DANASIM_RENDERER",          "csv")
DEPTH_PROVIDER_TYPE  = _get("renderer", "depth_provider", "DANASIM_DEPTH_PROVIDER",    "palette")
TERRAIN_LAYER_ID     = "topo_bathy"
IDLE_SLEEP_SECONDS   = 0.1
DEBUG_MODE           = True

# ---------------------------------------------------------------------------
# CSV renderer
# ---------------------------------------------------------------------------
CSV_WATER_THRESHOLD  = _get("csv", "water_threshold", "DANASIM_CSV_WATER_THRESHOLD", 0.0005)

# ---------------------------------------------------------------------------
# X3D renderer
# ---------------------------------------------------------------------------
X3D_SUBSAMPLE    = _get("x3d", "subsample",     "DANASIM_X3D_SUBSAMPLE",     1)
X3D_LOD_CHUNK    = _get("x3d", "lod_chunk",     "DANASIM_X3D_LOD_CHUNK",     256)
X3D_LOD_SUBSAMPLES = _cfg.get("x3d", {}).get("lod_subsamples", [1, 4, 16])
X3D_LOD_RANGES     = _cfg.get("x3d", {}).get("lod_ranges",     [5000.0, 20000.0])

# ---------------------------------------------------------------------------
# State mapping
# ---------------------------------------------------------------------------
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


def utc_now_iso() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")
