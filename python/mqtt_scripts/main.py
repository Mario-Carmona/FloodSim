
import time
import logging
import queue
import mqtt_scripts.config # Ya no importamos matplotlib aquí

# Import our new modules
from mqtt_scripts.data_model import SimulationGrid
from mqtt_scripts.network import MQTTMonitorClient
from mqtt_scripts.visualizer import GridVisualizer

logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - [%(levelname)s] - %(message)s'
)

def main():
    # 1. Crear la Cola (El puente entre hilos)
    msg_queue = queue.Queue()  # <--- NUEVO

    # 2. Inicializar Data Model
    simulation = SimulationGrid()

    # 3. Inicializar Red (Pasándole la COLA, no la simulación)
    mqtt_client = MQTTMonitorClient(msg_queue) # <--- CAMBIO
    mqtt_client.connect()

    # 4. Inicializar Visualizador
    viewer = GridVisualizer(output_folder="sim_outputs")
    
    print("Monitor iniciado. Esperando mensajes...")

    try:
        while True:
            # --- ESTE ES EL BUCLE SINCRONIZADO ---
            
            # A. Sacar mensaje de la cola. 
            # El programa se DETIENE aquí hasta que llega algo.
            x, y, step = msg_queue.get() 
            
            # B. Actualizar el modelo
            simulation.update_from_deltas(x, y, step)
            
            # C. Guardar la imagen (Bloqueante)
            # Hasta que esto no termine, no volvemos arriba a por otro mensaje
            viewer.save_snapshot(simulation.grid, step_index=step)
            
            # D. Marcar como completado
            msg_queue.task_done()
            
            print(f"Frame {step} procesado y guardado.")

    except KeyboardInterrupt:
        print("\nSaliendo...")
    finally:
        mqtt_client.disconnect()

if __name__ == "__main__":
    main()
