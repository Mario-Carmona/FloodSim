
import paho.mqtt.client as mqtt
import logging
from mqtt_scripts.libs import messages_pb2  # Assumes libs/messages_pb2.py exists
import mqtt_scripts.config

class MQTTMonitorClient:
    """
    Handles MQTT connections and deserializes incoming Protobuf messages.
    """

    def __init__(self, msg_queue):
        self.queue = msg_queue  # <--- GUARDAMOS LA COLA
        self._logger = logging.getLogger(__name__)
    
        # Configuración del cliente (Paho < 2.0.0)
        self.client = mqtt.Client(
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

    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            self._logger.info("Successfully connected to Broker.")
            client.subscribe(config.TOPIC_UPDATES)
        else:
            self._logger.error(f"Connection failed with code: {rc}")

    def _on_message(self, client, userdata, msg):
        try:
            # ... logs y parsing del frame igual ...
            frame = messages_pb2.SimulationFrame()
            frame.ParseFromString(msg.payload)

            # --- CAMBIO IMPORTANTE ---
            # NO actualizamos el modelo aquí.
            # Solo empaquetamos los datos y los metemos en la cola.
            packet = (list(frame.changed_x), list(frame.changed_y), frame.step_index)
        
            self.queue.put(packet) # <--- ENVIAMOS A LA COLA
            # -------------------------

        except Exception as e:
            self._logger.error(f"Error processing message: {e}", exc_info=True)
