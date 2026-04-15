
import time
import logging
import queue
import os
import sys

if __package__ in (None, ""):
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import config
    from data_model import SimulationGrid
    from network import MQTTMonitorClient
    from visualizer import GridVisualizer
else:
    from . import config
    from .data_model import SimulationGrid
    from .network import MQTTMonitorClient
    from .visualizer import GridVisualizer


logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - [%(levelname)s] - %(message)s'
)

def main():
    msg_queue = queue.Queue()

    simulation = SimulationGrid()

    mqtt_client = MQTTMonitorClient(msg_queue)
    mqtt_client.connect()

    viewer = GridVisualizer(output_folder="sim_outputs")

    print(f"Monitor iniciado para escenario '{config.SCENARIO_NAME}'. Esperando mensajes...")

    try:
        while True:
            item = msg_queue.get()

            if isinstance(item, tuple) and len(item) == 3:
                x, y, step = item
                simulation.update_from_deltas(x, y, step)
                viewer.save_snapshot(simulation.grid, step_index=step)
                print(f"Frame {step} procesado y guardado.")
                msg_queue.task_done()
                continue

            payload = item.get("payload", {})
            process = payload.get("process")

            if process == "EYE_SetState_Layer":
                simulation.update_from_layer_event(payload)
                viewer.save_snapshot(simulation.grid)
                print("Evento EYE_SetState_Layer procesado y renderizado.")
            elif process in {"InitMap_Config", "InitAgent_Layer", "Init_EOF", "Sim_End", "System_Disconnected"}:
                logging.info("Evento recibido: %s", process)
            else:
                logging.debug("Evento ignorado (no soportado todavía): %s", process)

            msg_queue.task_done()

    except KeyboardInterrupt:
        print("\nSaliendo...")
    finally:
        mqtt_client.disconnect()

if __name__ == "__main__":
    main()
