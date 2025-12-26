
import paho.mqtt.client as mqtt
import logging
from libs import messages_pb2  # Assumes libs/messages_pb2.py exists
import config

class MQTTMonitorClient:
    """
    Handles MQTT connections and deserializes incoming Protobuf messages.
    """

    def __init__(self, simulation_grid):
        self.grid_model = simulation_grid
        self._logger = logging.getLogger(__name__)
        
        # Configure the MQTT Client.
        # clean_session=True ensures we don't receive stale messages from previous runs.
        self.client = mqtt.Client(
            callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
            client_id=config.CLIENT_ID,
            clean_session=True
        )

        # Attach event callbacks
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message

    def connect(self):
        try:
            self.client.connect(config.BROKER_ADDRESS, config.BROKER_PORT, 60)
            self.client.loop_start()  # Run the network loop in a background thread
            self._logger.info(f"Connecting to broker at {config.BROKER_ADDRESS}...")
        except Exception as e:
            self._logger.critical(f"Failed to connect to MQTT broker: {e}")

    def disconnect(self):
        self.client.loop_stop()
        self.client.disconnect()
        self._logger.info("Disconnected from MQTT broker.")

    def _on_connect(self, client, userdata, flags, rc, properties):
        if rc == 0:
            self._logger.info("Successfully connected to Broker.")
            client.subscribe(config.TOPIC_UPDATES)
        else:
            self._logger.error(f"Connection failed with code: {rc}")

    def _on_message(self, client, userdata, msg):
        """
        Callback for when a message is received. 
        Parses Protobuf and delegates logic to the data model.
        """
        try:
            if config.DEBUG_MODE:
                self._logger.debug(f"Payload received. Size: {len(msg.payload)} bytes")

            # Parse the Protobuf message
            frame = messages_pb2.SimulationFrame()
            frame.ParseFromString(msg.payload)

            # Update the Data Model
            # We pass the raw repeated fields (changed_x, changed_y) to the model.
            self.grid_model.update_from_deltas(
                frame.changed_x, 
                frame.changed_y, 
                frame.step_index
            )

        except Exception as e:
            self._logger.error(f"Error processing message: {e}", exc_info=True)
