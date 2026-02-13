# -*- coding: utf-8 -*-

import os
import tensorflow as tf

# Forzar CPU para exportación limpia (opcional, pero recomendado)
os.environ["CUDA_VISIBLE_DEVICES"] = "-1"

class SimulatorModule(tf.Module):
    def __init__(self):
        super(SimulatorModule, self).__init__()

    @tf.function(input_signature=[
        tf.TensorSpec(shape=[None, None, None, 1], dtype=tf.float32, name="water_depth"),
        tf.TensorSpec(shape=[None, None, None, 1], dtype=tf.float32, name="elevation"),
        tf.TensorSpec(shape=[None, None, None, 1], dtype=tf.float32, name="roughness"),
        tf.TensorSpec(shape=[None, None, None, 1], dtype=tf.float32, name="rainfall"),
        tf.TensorSpec(shape=[], dtype=tf.float32, name="dt"),
        tf.TensorSpec(shape=[], dtype=tf.float32, name="dx")
    ])
    def run_simulation_batch(self, water_depth, elevation, roughness, rainfall, dt, dx):
        """
        Ejecuta 'steps' iteraciones de la simulación.
        Args:
            water_depth: Tensor (Batch, H, W, 1) - Estado inicial del agua
            elevation:   Tensor (Batch, H, W, 1) - Elevación del terreno
            roughness:   Tensor (Batch, H, W, 1) - Rugosidad
            rainfall:    Tensor (Batch, H, W, 1) - Precipitaciones
            steps:       Scalar int64 - Número de pasos a simular
        Returns:
            water_depth_next:  Tensor (Batch, H, W, 1) - Estado final del agua
        """

        # Constantes pre-calculadas para las 8 direcciones (N, S, W, E, NW, NE, SW, SE)
        # Distancias: Cardinales=1.0, Diagonales=1.414
        # Las elevamos a 1.5 aquí mismo para no hacerlo en el bucle: 1.0^1.5=1.0, 1.414^1.5=1.68
        dist_factors_pow = tf.constant([1.0, 1.0, 1.0, 1.0, 1.6817, 1.6817, 1.6817, 1.6817], dtype=tf.float32)
        dist_factors_pow = tf.reshape(dist_factors_pow, [1, 1, 1, 8])

        # 1. Preparar Estado
        water_depth_with_rain = water_depth + rainfall
        head = elevation + water_depth_with_rain

        # ---------------------------------------------------------
        # STACKING DE VECINOS (Solo 1 operación de padding)
        # ---------------------------------------------------------
        # Pad CONSTANT -10000 en elevación simula precipicio
        # Para hacerlo vectorizado, paddeamos 'head' directamente.
        paddings = [[0, 0], [1, 1], [1, 1], [0, 0]]
        pad_head = tf.pad(head, paddings, mode='CONSTANT', constant_values=-10000.0)
        
        # Extraer vecinos (Slicing es prácticamente gratis en TF/ONNX)
        neighbors = tf.concat([
            pad_head[:, 0:-2, 1:-1, :], # N
            pad_head[:, 2:  , 1:-1, :], # S
            pad_head[:, 1:-1, 0:-2, :], # W
            pad_head[:, 1:-1, 2:  , :], # E
            pad_head[:, 0:-2, 0:-2, :], # NW
            pad_head[:, 0:-2, 2:  , :], # NE
            pad_head[:, 2:  , 0:-2, :], # SW
            pad_head[:, 2:  , 2:  , :]  # SE
        ], axis=3) # [Batch, H, W, 8]

        # ---------------------------------------------------------
        # CÁLCULO FÍSICO "FAST MATH"
        # ---------------------------------------------------------
        
        # 1. Gradientes (Vectorizado)
        # Solo consideramos flujo si head > neighbor_head
        diff = head - neighbors
        gradients = tf.maximum(diff, 0.0) 

        # 2. Factor de Agua (Sustitución de POW)
        # Original: h^1.666
        # Optimizado: h * sqrt(h)  (= h^1.5)
        # Esto elimina la operación trascendental 'pow'.
        sqrt_water = tf.sqrt(water_depth_with_rain)
        water_factor = water_depth_with_rain * sqrt_water 
        
        # 3. Denominador (Fricción y Distancia)
        # Flow = (WaterFactor * sqrt(Grad) * dt) / (Rough * Dist^1.5)
        # Reordenamos para multiplicar: Flow = (WaterFactor * dt / Rough) * (sqrt(Grad) / Dist^1.5)
        
        # Pre-cálculo del término de celda común
        # Usamos maximum(roughness, 1e-4) para evitar división por cero
        cell_base = (water_factor * dt) / tf.maximum(roughness, 1e-4)
        
        # Término de distancia (dx^1.5 * factores_precalculados)
        # dx es escalar, así que sqrt(dx) es barato
        dx_pow = dx * tf.sqrt(dx)
        dist_term = dx_pow * dist_factors_pow

        # 4. Flujo Bruto
        # Usamos sqrt(gradients). Como gradients >= 0, es seguro.
        outflows = cell_base * (tf.sqrt(gradients) / dist_term)

        # --- LIMITADOR ---
        total_out = tf.reduce_sum(outflows, axis=3, keepdims=True)
        
        # Escalar si total_out > agua disponible
        # Usamos una multiplicación segura en vez de división condicional compleja
        scale = tf.minimum(1.0, water_depth_with_rain / (total_out + 1e-9))
        
        real_outflows = outflows * scale
        total_out_real = total_out * scale # = reduce_sum(real_outflows)

        # ---------------------------------------------------------
        # INTERCAMBIO DE FLUJOS (INFLOWS)
        # ---------------------------------------------------------
        # Paddeamos con 0 para que del vacío no entre nada
        pad_flows = tf.pad(real_outflows, paddings, mode='CONSTANT', constant_values=0.0)

        # Recuperar flujos opuestos (Swapping de canales)
        # N(0) recibe de S(1), S(1) recibe de N(0), etc.
        inflows = (
            pad_flows[:, 0:-2, 1:-1, 1:2] + # Vecino N (canal S)
            pad_flows[:, 2:  , 1:-1, 0:1] + # Vecino S (canal N)
            pad_flows[:, 1:-1, 0:-2, 3:4] + # Vecino W (canal E)
            pad_flows[:, 1:-1, 2:  , 2:3] + # Vecino E (canal W)
            pad_flows[:, 0:-2, 0:-2, 7:8] + # Vecino NW (canal SE)
            pad_flows[:, 0:-2, 2:  , 6:7] + # Vecino NE (canal SW)
            pad_flows[:, 2:  , 0:-2, 5:6] + # Vecino SW (canal NE)
            pad_flows[:, 2:  , 2:  , 4:5]   # Vecino SE (canal NW)
        )

        # Balance final
        new_water = water_depth_with_rain + (inflows - total_out_real)
        
        # Asegurar no negativos (por errores de precisión float32)
        return tf.maximum(new_water, 0.0)
