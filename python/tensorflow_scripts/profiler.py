# -*- coding: utf-8 -*-

import tensorflow as tf
import numpy as np
import os
import datetime

from tensorflow_scripts.simulator_module import SimulatorModule
from tensorflow_scripts.constants_loader import load_simulation_constants


# --- CONFIGURACIÓN ---
ROWS, COLS = 512, 512  # Aumentado para ver carga real
STEPS_TO_PROFILE = 10  # Pasos a medir
WARMUP_STEPS = 5

CPP_CONSTS = load_simulation_constants()


def get_real_data():
    """Genera datos sintéticos pesados."""
    # Creamos un escenario complejo: terreno ondulado y agua en el centro
    x = np.linspace(0, 20, COLS)
    y = np.linspace(0, 20, ROWS)
    xv, yv = np.meshgrid(x, y)
    elev = (np.sin(xv) * 5.0 + np.cos(yv) * 5.0).astype(np.float32).reshape(1, ROWS, COLS, 1)
    
    # Bloque de agua central
    water = np.zeros((1, ROWS, COLS, 1), dtype=np.float32)
    water[:, ROWS//2-50:ROWS//2+50, COLS//2-50:COLS//2+50, :] = 2.0
    
    # Estado: Donde hay agua es dinámico
    state = np.zeros((1, ROWS, COLS, 1), dtype=np.int8)
    state[water > 0] = CPP_CONSTS['STATE_DYNAMIC']
    
    return elev, water, state


def main():
    log_base_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "logs")
    current_log_dir = os.path.join(log_base_dir, datetime.datetime.now().strftime("%Y%m%d-%H%M%S"))

    # 1. Instanciar
    model = SimulatorModule()

    elev_np, water_np, state_np = get_real_data()

    elev = tf.constant(elev_np)
    water = tf.constant(water_np)
    state = tf.constant(state_np)
    rough = tf.zeros_like(elev)
    rain = tf.zeros_like(water)

    print("1. Inicializando...")
    model.initialize_state(elev, rough, 1.0, 1.0, 3, 0.0, 512.0)

    # ---------------------------------------------------------
    # FASE 1: EXPORTACIÓN DEL GRAFO (GRAPH VIEW)
    # ---------------------------------------------------------
    print("2. Trazando estructura del grafo...")
    # Activamos el trazador de grafo justo antes del warmup
    writer = tf.summary.create_file_writer(current_log_dir)
    tf.summary.trace_on(graph=True)

    # Ejecutamos una vez para que TF construya el grafo
    model.simulation_loop(water, state, rain, 0.0, 512.0, 0.0, 512.0)

    with writer.as_default():
        # Esto guarda el archivo que dibuja las cajas en TensorBoard
        tf.summary.trace_export(name="Graph_Structure", step=0, profiler_outdir=current_log_dir)
    print("   Estructura exportada.")

    # ---------------------------------------------------------
    # FASE 2: PROFILING DE EJECUCIÓN (TIMELINE)
    # ---------------------------------------------------------
    print(f"3. Iniciando Profiler Profesional en: {current_log_dir}")
    
    # Configuración máxima de detalle
    options = tf.profiler.experimental.ProfilerOptions(
        host_tracer_level=2,       # Detalle CPU
        python_tracer_level=1,     # Detalle Python
        device_tracer_level=1      # Detalle GPU (si aplica)
    )

    with tf.profiler.experimental.Profile(current_log_dir, options=options):
        for i in range(STEPS_TO_PROFILE):
            # "Trace" crea un bloque con nombre en la línea de tiempo
            # para que sepas dónde empieza y acaba cada llamada Python.
            with tf.profiler.experimental.Trace('Step_Simulacion', step_num=i):
                model.simulation_loop(water, state, rain, 0.0, 512.0, 0.0, 512.0)

    print(">> HECHO. Ejecuta:")
    print(f"   tensorboard --logdir={log_base_dir}")


if __name__ == "__main__":
    main()
