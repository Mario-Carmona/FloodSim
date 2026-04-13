
"""
config.py
Central configuration file for the Simulation Monitor.
"""

# Simulation constraints
MAP_SIZE = 9403  # Grid dimensions (20x20)

# MQTT Broker Configuration
BROKER_ADDRESS = "localhost"
BROKER_PORT = 1883
TOPIC_UPDATES = "danasim/updates"
CLIENT_ID = "Danasim_Monitor_Viewer"

# Visualization settings
REFRESH_RATE_SECONDS = 0.1  # Interval for UI updates
IDLE_SLEEP_SECONDS = 0.1    # CPU saver when no data is arriving
DEBUG_MODE = True
