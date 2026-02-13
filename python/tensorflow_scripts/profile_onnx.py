import onnxruntime as ort
import numpy as np
import time
import os

# --- CONFIGURACIÓN ---
MODEL_PATH = "models/simulation_model_cpu.onnx"
# Prueba con el tamaño de mapa que usarás en el juego
HEIGHT = 256
WIDTH = 256
STEPS_PER_CALL = 1  # Cuántos pasos simula el modelo por cada llamada .run()

def run_profile():
    print(f"--- Iniciando Profiling para mapa {HEIGHT}x{WIDTH} ---")
    
    # 1. Configurar Sesión con opciones de optimización
    sess_options = ort.SessionOptions()
    
    # Nivel de optimización del grafo (fusión de nodos, eliminación de constantes)
    sess_options.graph_optimization_level = ort.GraphOptimizationLevel.ORT_ENABLE_ALL
    
    # Habilitar el Profiler interno (Genera un JSON al final)
    sess_options.enable_profiling = True
    
    # Usar CPU (Tu Xeon)
    providers = ['CPUExecutionProvider']
    
    print("1. Cargando modelo ONNX...")
    try:
        session = ort.InferenceSession(MODEL_PATH, sess_options, providers=providers)
    except Exception as e:
        print(f"Error cargando modelo: {e}")
        return

    # 2. Generar datos dummy (Ruido aleatorio)
    # Importante: Usar float32 explícitamente
    shape = (1, HEIGHT, WIDTH, 1)
    
    inputs = {
        "water_depth": np.random.rand(*shape).astype(np.float32),
        "elevation":   np.random.rand(*shape).astype(np.float32),
        "roughness":   np.zeros(shape, dtype=np.float32),
        "rainfall":    np.zeros(shape, dtype=np.float32),
        "dt":          np.array(1.0, dtype=np.float32),
        "dx":          np.array(1.0, dtype=np.float32),
        "steps":       np.array(STEPS_PER_CALL, dtype=np.int64) # O int64 según tu export
    }

    # 3. WARMUP (Calentamiento)
    # Las primeras ejecuciones siempre son lentas (allocations, cacheo). No se miden.
    print("2. Calentando motores (Warmup 10 iters)...")
    for _ in range(10):
        session.run(None, inputs)

    # 4. BENCHMARK REAL
    ITERATIONS = 100
    print(f"3. Ejecutando {ITERATIONS} iteraciones de medición...")
    
    start_time = time.time()
    for _ in range(ITERATIONS):
        # Aquí ocurre la magia
        new_water = session.run(None, inputs)
        
        # Opcional: Para simular realidad, realimentamos la salida como entrada
        # inputs["water_depth"] = new_water[0] 
    
    end_time = time.time()

    # 5. RESULTADOS
    total_time = end_time - start_time
    avg_time = total_time / ITERATIONS
    fps = 1.0 / avg_time
    
    print("\n" + "="*40)
    print(f" RESULTADOS DE RENDIMIENTO (Xeon CPU)")
    print("="*40)
    print(f" Mapa:          {HEIGHT}x{WIDTH} celdas")
    print(f" Tiempo Total:  {total_time:.4f} s")
    print(f" Tiempo Medio:  {avg_time*1000:.2f} ms por iteración")
    print(f" Velocidad:     {fps:.2f} iteraciones/segundo")
    print("="*40)

    # Cerrar profiling para que guarde el archivo json
    prof_file = session.end_profiling()
    print(f"\nArchivo de detalle generado: {prof_file}")
    print("Sube este archivo a https://ui.perfetto.dev/ para ver el gráfico.")

if __name__ == "__main__":
    run_profile()