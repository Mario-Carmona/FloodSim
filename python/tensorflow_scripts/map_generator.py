
# map_generator.py
import tensorflow as tf
import numpy as np

def generate_synthetic_scenario(rows, cols, active_pct_target=0.5):
    """
    Genera un mapa con:
    1. Una pendiente suave (para que el agua fluya).
    2. Obstáculos (conos/montañas) para forzar cálculos de flujo complejos.
    3. Una cantidad exacta de agua inicial basada en active_pct_target.
    """
    
    # 1. COORDENADAS
    x = tf.linspace(-10.0, 10.0, cols)
    y = tf.linspace(-10.0, 10.0, rows)
    X, Y = tf.meshgrid(x, y)
    
    # 2. ELEVACIÓN (Pendiente + Obstáculos)
    # Pendiente general hacia abajo
    base_slope = X * 0.5 
    # Obstáculos (función seno/coseno para crear "montañas")
    obstacles = tf.sin(X) * tf.cos(Y) * 2.0
    
    elevation = base_slope + obstacles
    
    # Expandir dimensiones a (Batch, H, W, Channels)
    elevation = elevation[tf.newaxis, ..., tf.newaxis]
    
    # 3. ROUGHNESS (Rugosidad variable)
    # Hacemos que algunas zonas sean más rugosas para variar la fricción
    roughness = tf.ones_like(elevation) * 0.03

    obstacles_expanded = obstacles[tf.newaxis, ..., tf.newaxis]
    roughness = tf.where(obstacles_expanded > 0.5, 0.06, roughness) # Montañas son rugosas
    
    # 4. AGUA (Control preciso de Celdas Activas)
    # Estrategia: "Dam Break". Llenamos la parte superior del mapa hasta cumplir el %.
    total_cells = rows * cols
    target_water_cells = int(total_cells * active_pct_target)
    
    # Creamos una máscara plana
    flat_indices = np.arange(total_cells)
    # Llenamos las primeras N celdas (que corresponden a la parte "izquierda/arriba" por el meshgrid)
    # o simplemente llenamos aleatoriamente para distribuir la carga si prefieres "Active Cells Dispersas"
    # Para simulación de fluidos, es mejor bloques continuos:
    water_flat = np.zeros(total_cells, dtype=np.float32)
    water_flat[:target_water_cells] = 5.0 # 5 metros de profundidad
    
    # Hacemos shuffle si queremos probar "scattered load" o lo dejamos así para "wave load"
    # np.random.shuffle(water_flat) 
    
    water = tf.cast(tf.reshape(water_flat, (1, rows, cols, 1)), tf.float32)

    # Rainfall cero para este test
    rain = tf.zeros_like(water)

    return elevation, water, roughness, rain
