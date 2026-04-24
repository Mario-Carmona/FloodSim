
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

    viewer = GridVisualizer(output_folder=os.getenv("DANASIM_OUTPUT_FOLDER", "sim_outputs"))

    running = True
    pending_changes: list = []
    chunks_per_batch = 0
    chunks_since_ack = 0

    def render():
        if not simulation.initialization["init_complete"]:
            return
        if not simulation.has_new_data:
            return
        viewer.save_snapshot(simulation.grid)
        simulation.consume_data()

    print(f"Monitor iniciado para escenario '{config.SCENARIO_NAME}'. Esperando mensajes...")

    try:
        while running:
            try:
                item = msg_queue.get(timeout=config.IDLE_SLEEP_SECONDS)
            except queue.Empty:
                continue

            payload = item.get("payload", {})
            process = payload.get("process")

            if process == "InitMap_Config":
                ok = simulation.apply_init_map_config(payload)
                if ok:
                    logging.info("InitMap_Config aplicado. Grid %sx%s", simulation.grid.shape[1], simulation.grid.shape[0])
                else:
                    logging.error("InitMap_Config inválido")
            elif process == "InitAgent_Layer":
                ok = simulation.apply_init_agent_layer(payload)
                if ok:
                    logging.info(
                        "InitAgent_Layer aplicado. Capas init recibidas: %s",
                        simulation.initialization["init_layers_received"],
                    )
                else:
                    logging.error("No se pudo aplicar InitAgent_Layer")
            elif process == "InitAgent_EOF":
                simulation.mark_init_agent_eof(payload)
            elif process == "Init_EOF":
                simulation.mark_init_eof(payload)
                if config.RENDER_ON_INIT_EOF:
                    viewer.save_snapshot(simulation.grid)
                    simulation.consume_data()
                    print("Inicializacion completada. Snapshot inicial guardado.")
            elif process == "FrameStart":
                pending_changes.clear()
                chunks_per_batch = payload.get("chunks_per_batch", 0)
                chunks_since_ack = 0
                logging.info("FrameStart: total_chunks=%d, chunks_per_batch=%d", payload.get("total_chunks", 0), chunks_per_batch)
            elif process == "FrameEnd":
                n = len(pending_changes)
                simulation.apply_bulk_changes(pending_changes)
                pending_changes.clear()
                logging.info("FrameEnd: %d cambios aplicados al grid.", n)
            elif process == "EYE_SetState_Layer":
                pending_changes.extend(simulation.collect_from_layer_event(payload))
                if chunks_per_batch > 0:
                    chunks_since_ack += 1
                    if chunks_since_ack >= chunks_per_batch:
                        mqtt_client.publish_chunk_ack()
                        chunks_since_ack = 0
            elif process == "EYE_SetState":
                pending_changes.extend(simulation.collect_from_object_event(payload))
                if chunks_per_batch > 0:
                    chunks_since_ack += 1
                    if chunks_since_ack >= chunks_per_batch:
                        mqtt_client.publish_chunk_ack()
                        chunks_since_ack = 0
            elif process == "EYE_Frame_Sync":
                render()
                logging.info(
                    "EYE_Frame_Sync: imagen generada. simulation_time=%s",
                    payload.get("simulation_time"),
                )
            elif process == "Sim_End":
                logging.info("Sim_End recibido. Cerrando.")
                running = False
            elif process == "System_Disconnected":
                logging.info("Evento recibido: %s", process)
            else:
                logging.debug("Evento ignorado: %s", process)

            msg_queue.task_done()

    except KeyboardInterrupt:
        print("\nSaliendo...")
    finally:
        mqtt_client.disconnect()
        print("Clean exit after Sim_End", flush=True)
        sys.exit(0)

if __name__ == "__main__":
    main()
