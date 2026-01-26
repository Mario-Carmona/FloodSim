# -*- coding: utf-8 -*-


import matplotlib.pyplot as plt
import io

def plot_memory_curve(history_list, step_number):
    """
    history_list: Lista de tuplas [(tiempo_ms, mem_mb), ...]
    Retorna un Tensor de imagen listo para tf.summary.image
    """
    # Separar datos
    times = [t for (t, m) in history_list]
    mems = [m for (t, m) in history_list]
    
    # 1. Crear gráfico con Matplotlib (Backend 'Agg' para no necesitar pantalla)
    plt.switch_backend('Agg') 
    figure = plt.figure(figsize=(10, 6))
    plt.plot(times, mems, label='Uso de RAM', color='cyan')
    
    # Decoración del gráfico
    plt.title(f'Perfil de Memoria - Step {step_number}')
    plt.xlabel('Tiempo (ms)')
    plt.ylabel('Memoria (MB)')
    plt.grid(True, linestyle='--', alpha=0.6)
    plt.legend()
    
    # Marcar el pico
    max_mem = max(mems)
    max_idx = mems.index(max_mem)
    plt.annotate(f'Pico: {max_mem:.1f} MB', 
                 xy=(times[max_idx], max_mem), 
                 xytext=(times[max_idx], max_mem + (max_mem*0.01)),
                 arrowprops=dict(facecolor='red', shrink=0.05))

    # 2. Guardar en buffer de memoria (RAM)
    buf = io.BytesIO()
    plt.savefig(buf, format='png')
    plt.close(figure)
    buf.seek(0)

    # 3. Convertir a Tensor
    # Decodificar PNG a Tensor uint8
    image = tf.image.decode_png(buf.getvalue(), channels=4)
    # Añadir dimensión de batch (1, H, W, C) requerida por TensorBoard
    image = tf.expand_dims(image, 0)
    
    return image



import threading
import time
import psutil
import os

class DetailedMemoryProfiler:
    def __init__(self, interval=0.001):
        """
        interval: Segundos entre muestras (0.001 = 1ms).
        """
        self.interval = interval
        self.process = psutil.Process(os.getpid())
        self.history = []  # Lista de tuplas (tiempo_ms, memoria_mb)
        self.running = False
        self.thread = None
        self.start_time = 0.0

    def _monitor(self):
        while self.running:
            # Capturamos tiempo relativo al inicio del step
            current_t = (time.perf_counter() - self.start_time) * 1000.0 # ms
            
            # Capturamos memoria actual en MB
            mem_mb = self.process.memory_info().rss / (1024 * 1024)
            
            self.history.append((current_t, mem_mb))
            time.sleep(self.interval)

    def start(self):
        self.running = True
        self.history = [] # Limpiar historial anterior
        self.start_time = time.perf_counter()
        
        self.thread = threading.Thread(target=self._monitor, daemon=True)
        self.thread.start()

    def stop(self):
        self.running = False
        if self.thread:
            self.thread.join()
        return self.history  # Retorna la lista completa




import tensorflow as tf
import time
import os
import argparse
import psutil
import numpy as np

from tensorflow_scripts.simulator_module import SimulatorModule
from tensorflow_scripts.map_generator import generate_synthetic_scenario

# --- CONFIGURACIÓN DE ESCENARIOS ---
# Aquí defines tu batería de pruebas
SCENARIOS = [
    {"rows": 5577, "cols": 9403, "active": 0.2, "tag": "Current"}
]

STEPS_PER_SCENARIO = 10
BATCH = 10
LOG_DIR = "logs/benchmark_suite"

def get_process_memory_mb():
    """Retorna memoria RAM física usada SOLO por este proceso."""
    process = psutil.Process(os.getpid())
    return process.memory_info().rss / (1024 * 1024)


def run_single_scenario(version_label, scenario_cfg):
    rows = scenario_cfg["rows"]
    cols = scenario_cfg["cols"]
    pct = scenario_cfg["active"]
    tag = scenario_cfg["tag"]

    # Nombre único para TensorBoard
    run_name = f"{version_label}_{tag}_{rows}x{cols}_Act{int(pct*100)}"
    log_base_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), LOG_DIR)
    log_path = os.path.join(log_base_dir, run_name)
    
    if os.path.exists(log_path):
        raise FileExistsError(
            f"\n\n🛑 ERROR: La carpeta '{log_path}' YA EXISTE.\n"
            f"   Por favor, cambia el nombre de la versión o borra la carpeta.\n"
        )

    print(f"\n>>> INICIANDO ESCENARIO: {run_name}")
    print(f"    Map: {rows}x{cols} | Active Water: {pct*100}%")

    # 1. Generar Mapa Perfecto
    elev, water, rough, rain = generate_synthetic_scenario(rows, cols, active_pct_target=pct)
    
    # 2. Inicializar Modelo
    model = SimulatorModule()
    # Asumiendo params: dt=0.1, dx=1.0, batch=1...
    model.initialize_state(elev, rough, water, rain, 1.0, 1.0, BATCH, 0.0, float(rows))

    # 4. Loop Medido
    writer = tf.summary.create_file_writer(log_path)
    accumulated_time_ms = 0.0
    TOTAL_CELLS = rows * cols

    profiler = DetailedMemoryProfiler(interval=0.0005)

    with writer.as_default():
        for step in range(STEPS_PER_SCENARIO):
            print(f"    >> Step: {step} ...")

            t0 = time.process_time()
            profiler.start()
            
            # Ejecutar
            model.run_simulation_batch()

            memory_curve = profiler.stop()
            t1 = time.process_time()
            
            # --- Cálculos ---
            step_time_ms = (t1 - t0) * 1000.0
            accumulated_time_ms += step_time_ms
            
            # 1. THROUGHPUT (La métrica reina para comparar tamaños distintos)
            if step_time_ms > 0:
                mcells_sec = (TOTAL_CELLS / (step_time_ms / 1000.0)) / 1e6
                tf.summary.scalar("1_Engineering/Throughput_Mcells_sec", mcells_sec, step=step)
            
            # 2. CARGA REAL (Validar que el generador funcionó)
            tf.summary.scalar("1_Engineering/Active_Cells_Pct", pct*100, step=step)

            # 3. TIEMPO
            tf.summary.scalar("2_Time/Step_ms", step_time_ms, step=step)
            tf.summary.scalar("2_Time/Accumulated_ms", accumulated_time_ms, step=step)
            
            # 4. MEMORIA
            if memory_curve:
                mem_values = [m for (t, m) in memory_curve]
                peak_mem = max(mem_values)
                
                # A. MÉTRICAS SIMPLES (Para ver la tendencia general)
                tf.summary.scalar("3_Memory/Peak_MB", peak_mem, step=step)
                
                # B. EL GRÁFICO DETALLADO (Lo que pediste)
                # Generamos la imagen solo si hay datos
                if len(mem_values) > 1:
                    img_tensor = plot_memory_curve(memory_curve, step)
                    tf.summary.image("3_Memory/IntraStep_Profile", img_tensor, step=step)

    print(f"    Finalizado. Logs en: {log_path}")
    
    # --- LIMPIEZA NUCLEAR ---
    # 1. Borrar referencias de Python
    del model, elev, water, rough, rain
    
    # 2. Limpiar sesión de Keras/TF
    tf.keras.backend.clear_session()
    
    # 3. Resetear grafo global (para TF 1.x legacy o grafos implícitos)
    tf.compat.v1.reset_default_graph()
    
    # 4. Forzar Garbage Collector (Varias veces para asegurar ciclos de referencia)
    import gc
    gc.collect()
    gc.collect()
    gc.collect()
    
    # 5. (Opcional) Dormir 1 segundo para dar tiempo al SO a reclamar RAM
    time.sleep(1.0)
    
    print("    >> Memoria limpiada. Preparando siguiente escenario...\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("label", help="Nombre de la versión (ej: V1_Original, V2_XLA)")
    args = parser.parse_args()
    
    print(f"Ejecutando {len(SCENARIOS)} escenarios...")
    
    for sc in SCENARIOS:
        run_single_scenario(args.label, sc)
