
import logging
import queue
import os
import sys
import time

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

    last_render_time = time.monotonic()
    running = True
    ended_by_sim_end = False

    def render_if_needed(force: bool = False):
        nonlocal last_render_time
        if not simulation.initialization["init_complete"] and not force:
            return

        if not simulation.has_new_data and not force:
            return

        now = time.monotonic()
        if not force and (now - last_render_time) < config.REFRESH_RATE_SECONDS:
            return

        viewer.save_snapshot(simulation.grid)
        simulation.consume_data()
        last_render_time = now

    print(f"Monitor iniciado para escenario '{config.SCENARIO_NAME}'. Esperando mensajes...")

    try:
        while running:
            try:
                item = msg_queue.get(timeout=config.IDLE_SLEEP_SECONDS)
            except queue.Empty:
                render_if_needed()
                continue

            if isinstance(item, tuple) and len(item) == 3:
                x, y, step = item
                simulation.update_from_deltas(x, y, step)
                render_if_needed(force=True)
                print(f"Frame {step} procesado y guardado.")
                msg_queue.task_done()
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
                    render_if_needed(force=True)
                    print("Inicializacion completada. Snapshot inicial guardado.")
            elif process == "EYE_SetState_Layer":
                changed = simulation.apply_event(payload)
            elif process == "EYE_SetState":
                changed = simulation.apply_event(payload)
                if changed:
                    print("Evento EYE_SetState procesado.")
            elif process == "EYE_Frame_Sync":
                # End-of-frame marker: flush pending layer/object updates as one render.
                render_if_needed(force=True)
                print(
                    "Frame sincronizado. total_chunks_sent=",
                    payload.get("total_chunks_sent"),
                    "simulation_time=",
                    payload.get("simulation_time"),
                )
            elif process == "Sim_End":
                logging.info("Evento recibido: Sim_End")
                render_if_needed(force=True)
                ended_by_sim_end = True
                simulation.initialization["init_complete"] = True
                running = False
            elif process == "System_Disconnected":
                logging.info("Evento recibido: %s", process)
            else:
                logging.debug("Evento ignorado (no soportado todavía): %s", process)

            msg_queue.task_done()

            render_if_needed()

    except KeyboardInterrupt:
        print("\nSaliendo...")
    finally:
        if not ended_by_sim_end:
            render_if_needed(force=True)
        mqtt_client.disconnect()
        print("Clean exit after Sim_End", flush=True)
        sys.exit(0)

if __name__ == "__main__":
    main()
