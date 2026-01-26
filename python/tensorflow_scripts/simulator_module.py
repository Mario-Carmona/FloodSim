# -*- coding: utf-8 -*-

import os
import tensorflow as tf

from tensorflow_scripts.constants_loader import load_simulation_constants


# --- 1. FORZAR MODO CPU ---
# Al poner esto a "-1", TensorFlow cree que no hay tarjetas NVIDIA instaladas.
# Esto evita que busque las DLLs de CUDA y elimina todos esos warnings.
os.environ["CUDA_VISIBLE_DEVICES"] = "-1"


class SimulatorModule(tf.Module):
    def __init__(self):
        super(SimulatorModule, self).__init__()

        CPP_CONSTS = load_simulation_constants()

        # --- MODEL CONSTANTS ---
        self.WATER_EPSILON = tf.constant(CPP_CONSTS['WATER_EPSILON'], dtype=tf.float32)

        self.DANGER_THRESHOLD = tf.constant(CPP_CONSTS['DANGER_THRESHOLD'], dtype=tf.float32)
        
        # --- VARIABLES DE ESTADO (Persisten en memoria de GPU/CPU) ---
        self.v_elevation = tf.Variable([], dtype=tf.float32, shape=tf.TensorShape(None), validate_shape=False, name="v_elevation")
        self.v_roughness = tf.Variable([], dtype=tf.float32, shape=tf.TensorShape(None), validate_shape=False, name="v_roughness")
        self.v_water_depth = tf.Variable([], dtype=tf.float32, shape=tf.TensorShape(None), validate_shape=False, name="v_water_depth")
        self.v_rainfall = tf.Variable([], dtype=tf.float32, shape=tf.TensorShape(None), validate_shape=False, name="v_rainfall")

        self.v_last_rendered_water = tf.Variable([], dtype=tf.float32, shape=tf.TensorShape(None), validate_shape=False)

        self.v_dt = tf.Variable(0.0, dtype=tf.float32, name="v_dt")
        self.v_dx = tf.Variable(1.0, dtype=tf.float32, name="v_dx")
        
        # NUEVO: Variable para controlar cuántos pasos hacer por llamada (Batching)
        self.v_steps_batch = tf.Variable(1, dtype=tf.int64, name="v_steps_batch")

        # Variables para el origen UTM (se llenan en init)
        self.v_origin_x = tf.Variable(0.0, dtype=tf.float32)
        self.v_origin_y = tf.Variable(0.0, dtype=tf.float32)

        self.v_mask_kill_top    = tf.Variable([], dtype=tf.float32, shape=tf.TensorShape(None), validate_shape=False, name="v_mask_kill_top")
        self.v_mask_kill_bottom = tf.Variable([], dtype=tf.float32, shape=tf.TensorShape(None), validate_shape=False, name="v_mask_kill_bottom")
        self.v_mask_kill_left   = tf.Variable([], dtype=tf.float32, shape=tf.TensorShape(None), validate_shape=False, name="v_mask_kill_left")
        self.v_mask_kill_right  = tf.Variable([], dtype=tf.float32, shape=tf.TensorShape(None), validate_shape=False, name="v_mask_kill_right")


    @tf.function(input_signature=[
        tf.TensorSpec(shape=[1, None, None, 1], dtype=tf.float32, name="elevation"),
        tf.TensorSpec(shape=[1, None, None, 1], dtype=tf.float32, name="roughness"),
        tf.TensorSpec(shape=[1, None, None, 1], dtype=tf.float32, name="water_depth"),
        tf.TensorSpec(shape=[1, None, None, 1], dtype=tf.float32, name="rainfall"),
        tf.TensorSpec(shape=[],                 dtype=tf.float32, name="dt"),
        tf.TensorSpec(shape=[],                 dtype=tf.float32, name="dx"),
        tf.TensorSpec(shape=[],                 dtype=tf.int64,   name="steps_batch"),
        tf.TensorSpec(shape=[],                 dtype=tf.float32, name="map_origin_x"),
        tf.TensorSpec(shape=[],                 dtype=tf.float32, name="map_origin_y")
    ])
    def initialize_state(self, elevation, roughness, water_depth, rainfall, dt, dx, steps_batch, map_origin_x, map_origin_y):
        self.v_elevation.assign(elevation)
        self.v_roughness.assign(roughness)
        self.v_water_depth.assign(water_depth)
        self.v_rainfall.assign(rainfall)

        self.v_last_rendered_water.assign(tf.zeros_like(water_depth))

        self.v_dt.assign(dt)
        self.v_dx.assign(dx)
        self.v_steps_batch.assign(steps_batch)
        self.v_origin_x.assign(map_origin_x)
        self.v_origin_y.assign(map_origin_y)


        # --- OPTIMIZACIÓN DE BORDES (Vectores de Broadcasting) ---
        # En lugar de máscaras 2D gigantes, usamos vectores 1D.
        # Shape del mapa: (Batch, Alto, Ancho, Canales) -> (1, H, W, 1)

        # 1. Extraer dimensiones dinámicas (Tensores enteros)
        shape_tensor = tf.shape(elevation)
        H = shape_tensor[1]
        W = shape_tensor[2]

        # 1. Vectores Verticales
        m_top = tf.concat([tf.zeros([1, 1]), tf.ones([H - 1, 1])], axis=0)
        # CAMBIO AQUÍ: Usar .assign
        self.v_mask_kill_top.assign(tf.reshape(m_top, [1, H, 1, 1]))

        m_btm = tf.concat([tf.ones([H - 1, 1]), tf.zeros([1, 1])], axis=0)
        # CAMBIO AQUÍ: Usar .assign
        self.v_mask_kill_bottom.assign(tf.reshape(m_btm, [1, H, 1, 1]))

        # 2. Vectores Horizontales
        m_left = tf.concat([tf.zeros([1, 1]), tf.ones([W - 1, 1])], axis=0)
        # CAMBIO AQUÍ: Usar .assign
        self.v_mask_kill_left.assign(tf.reshape(m_left, [1, 1, W, 1]))

        m_right = tf.concat([tf.ones([W - 1, 1]), tf.zeros([1, 1])], axis=0)
        # CAMBIO AQUÍ: Usar .assign
        self.v_mask_kill_right.assign(tf.reshape(m_right, [1, 1, W, 1]))


        # Importante: Retornar algo y nombrarlo para encontrarlo en C++
        return tf.identity(tf.constant(1.0), name="init_output_signal")

    @tf.function(jit_compile=True, input_signature=[])
    def run_simulation_batch(self):
        """
        Ejecuta N pasos de simulación (definidos en v_steps_batch)
        sin devolver el control a C++.
        """

        # Lectura de variables internas
        elevation = self.v_elevation.read_value()
        roughness = self.v_roughness.read_value()
        dt = self.v_dt.read_value()
        dx = self.v_dx.read_value()

        # 1. Leemos la configuración guardada
        current_water = self.v_water_depth.read_value()
        current_rainfall = self.v_rainfall.read_value()

        steps_batch = self.v_steps_batch

        def body(i, water_depth):
            # Pasamos elevation, roughness, etc. como args fijos
            new_water_depth = self.simulation_step(water_depth, current_rainfall, elevation, roughness, dt, dx)
            return i + 1, new_water_depth

        _, final_water_depth = tf.nest.map_structure(tf.stop_gradient, tf.while_loop(
            lambda i, water_depth: i < steps_batch,
            body,
            loop_vars=[tf.constant(0, dtype=tf.int64), current_water],
            parallel_iterations=1
        ))

        self.v_water_depth.assign(final_water_depth)
        
        # Output names forced via identity
        return tf.identity(tf.constant(1.0), name="run_output_signal")

    @tf.function(jit_compile=True)
    def simulation_step(self, water_depth, rainfall, elevation, roughness, dt, dx):
        # ---------------------------------------------------------
        # 1. APLICAR LLUVIA (Global)
        # ---------------------------------------------------------

        # APLICAR A TODO EL MAPA
        # Sumamos la lluvia directamente a la profundidad actual.
        # Esto acumulará agua incluso donde water_depth era 0.
        water_depth_with_rain = water_depth + rainfall

        # ---------------------------------------------------------
        # 3. HIDRÁULICA (Usando el nuevo depth y nuevo state)
        # ---------------------------------------------------------
        head = elevation + water_depth_with_rain

        # Acumuladores (Empezamos en 0)
        # Usaremos esto para ir sumando sin guardar 8 tensores en memoria simultáneamente
        sum_outflows = tf.zeros_like(water_depth_with_rain)
        net_flow = tf.zeros_like(water_depth_with_rain)

        # ---------------------------------------------------------
        # FUNCIÓN AUXILIAR INTERNA (Inline)
        # Calcula flujo hacia UNA dirección específica y actualiza acumuladores
        # ---------------------------------------------------------
        def process_direction(dy, dx, dist_factor):
            # 1. Obtener vecino con tf.roll (Más seguro que slice para mantener tamaño exacto)
            # Al hacerlo dentro de esta función, TF puede liberar 'neighbor_head' 
            # en cuanto termine esta llamada, ahorrando RAM.
            neighbor_head = tf.roll(head, shift=[dy, dx], axis=[1, 2])
            
            # Gradiente
            gradient = tf.maximum(head - neighbor_head, 0.0)

            # Manning
            distance = dist_factor * dx
            denom = (roughness + 1e-7) * tf.pow(distance, 1.5)
            
            # Flujo saliente
            flow = (tf.pow(water_depth_with_rain, 5.0/3.0) * tf.sqrt(gradient) * dt) / denom
            
            return flow

        # ---------------------------------------------------------
        # PROCESAMIENTO SECUENCIAL (Dirección por dirección)
        # ---------------------------------------------------------
        # Esto mantiene la "Memoria Activa" (Working Set) baja (~400MB)
        # permitiendo que entre en la caché de la CPU más fácilmente.
        
        # Lista de direcciones (dy, dx, dist)
        # Cardinales (dist=1.0)
        f_n = process_direction( 1,  0, 1.0) # Norte
        f_s = process_direction(-1,  0, 1.0) # Sur
        f_e = process_direction( 0,  1, 1.0) # Este
        f_w = process_direction( 0, -1, 1.0) # Oeste
        
        # Diagonales (dist=1.414)
        f_ne = process_direction( 1,  1, 1.414)
        f_nw = process_direction( 1, -1, 1.414)
        f_se = process_direction(-1,  1, 1.414)
        f_sw = process_direction(-1, -1, 1.414)

        # 1. Calcular Salida Total (Para el Limiter)
        # Sumamos todo. TF es listo y reutilizará buffers intermedios.
        total_potential_out = (f_n + f_s + f_e + f_w + f_ne + f_nw + f_se + f_sw)

        # 2. Calcular Limiter
        limiter = tf.where(total_potential_out > 0, 
                           tf.minimum(1.0, water_depth_with_rain / (total_potential_out + 1e-9)), 
                           1.0)

        # 3. Calcular Flujo Neto (Entrada - Salida)
        # Truco: Inflow = Roll Inverso del Outflow del vecino * Limiter del vecino
        # Como no tenemos el limiter del vecino, asumimos una aproximación local o
        # aplicamos el limiter al flujo saliente Y LUEGO hacemos el roll inverso.
        # (Esto es físicamente correcto: lo que realmente sale de mi vecino es flow * su_limiter)

        # Aplicamos limiter a los flujos que acabamos de calcular
        # Ahora f_n representa "Agua real que sale hacia el Norte"
        f_n *= limiter; f_s *= limiter; f_e *= limiter; f_w *= limiter
        f_ne *= limiter; f_nw *= limiter; f_se *= limiter; f_sw *= limiter

        # Acumulamos el flujo neto: (Lo que me llega) - (Lo que yo saco)
        
        # A. Lo que yo saco (Suma de mis flujos reales)
        total_out_real = (f_n + f_s + f_e + f_w + f_ne + f_nw + f_se + f_sw)
        net_flow -= total_out_real

        # B. Lo que me llega (Roll inverso de lo que mis vecinos mandan HACIA MI)
        # Si yo mandé al Norte (1,0), mi vecino del Norte me manda desde (-1,0)
        # Pero cuidado: f_n es "Yo -> Norte".
        # Necesito "Vecino Sur -> Yo". Eso es el f_n calculado por el vecino del sur.
        # Por tanto, hago shift del f_n global hacia el norte para traer lo que el sur mandó.
        
        # Recogemos inflows invirtiendo el shift sobre los flujos calculados
        # Inflow desde el SUR = Roll(Flow_hacia_Norte, shift=[-1, 0])
        # (El agua que iba al norte desde la celda de abajo, ahora está en mi celda)
        
        # --- LECTURA DE MÁSCARAS (Añadir al inicio o antes de usar) ---
        # Usamos .read_value() para obtener el tensor actual de la variable
        mask_top    = self.v_mask_kill_top.read_value()
        mask_bottom = self.v_mask_kill_bottom.read_value()
        mask_left   = self.v_mask_kill_left.read_value()
        mask_right  = self.v_mask_kill_right.read_value()

        net_flow += tf.roll(f_n, [-1, 0], axis=[1,2]) * mask_top # Viene del Sur (Flow que iba al Norte)
        net_flow += tf.roll(f_s, [ 1, 0], axis=[1,2]) * mask_bottom # Viene del Norte
        net_flow += tf.roll(f_e, [ 0,-1], axis=[1,2]) * mask_left # Viene del Oeste
        net_flow += tf.roll(f_w, [ 0, 1], axis=[1,2]) * mask_right # Viene del Este
        
        net_flow += tf.roll(f_ne, [-1,-1], axis=[1,2]) * mask_left # Viene del SO
        net_flow += tf.roll(f_nw, [-1, 1], axis=[1,2]) * mask_right # Viene del SE
        net_flow += tf.roll(f_se, [ 1,-1], axis=[1,2]) * mask_left # Viene del NO
        net_flow += tf.roll(f_sw, [ 1, 1], axis=[1,2]) * mask_right # Viene del NE

        # 4. ACTUALIZAR ESTADO
        output_water_depth = tf.maximum(water_depth_with_rain + net_flow, 0.0)

        return output_water_depth

    def get_view_indices(self, view_min_x, view_max_x, view_min_y, view_max_y, map_shape):
        rows_map = map_shape[1]
        cols_map = map_shape[2]

        # 1. Invertir formulas UTM -> Indices
        min_c_f = (view_min_x - self.v_origin_x) / self.v_dx
        max_c_f = (view_max_x - self.v_origin_x) / self.v_dx
        
        # Eje Y invertido
        min_r_f = (self.v_origin_y - view_max_y) / self.v_dx
        max_r_f = (self.v_origin_y - view_min_y) / self.v_dx

        # 2. Convertir a Enteros y Clampear (CORRECCIÓN AQUÍ: tf.math)
        min_c = tf.cast(tf.maximum(0.0, tf.math.floor(min_c_f)), tf.int32)
        max_c = tf.cast(tf.minimum(tf.cast(cols_map, tf.float32), tf.math.ceil(max_c_f)), tf.int32)
        
        min_r = tf.cast(tf.maximum(0.0, tf.math.floor(min_r_f)), tf.int32)
        max_r = tf.cast(tf.minimum(tf.cast(rows_map, tf.float32), tf.math.ceil(max_r_f)), tf.int32)

        # Validar consistencia
        min_c = tf.minimum(min_c, max_c)
        min_r = tf.minimum(min_r, max_r)

        return min_r, max_r, min_c, max_c

    @tf.function(jit_compile=False, input_signature=[
        tf.TensorSpec(shape=[], dtype=tf.float32, name="view_min_x"),
        tf.TensorSpec(shape=[], dtype=tf.float32, name="view_max_x"),
        tf.TensorSpec(shape=[], dtype=tf.float32, name="view_min_y"),
        tf.TensorSpec(shape=[], dtype=tf.float32, name="view_max_y")
    ])
    def get_view_changes(self, view_min_x, view_max_x, view_min_y, view_max_y):
        """
        Compara el estado actual (v_water_depth) con el último enviado (v_last_rendered_water).
        Retorna solo las coordenadas que han cambiado de estado crítico.
        """

        current_water = self.v_water_depth.read_value()

        last_sent_water = self.v_last_rendered_water.read_value()
        
        # 1. Lógica de Detección de Cambios (Tu lógica original adaptada)
        was_danger = tf.greater(last_sent_water, self.DANGER_THRESHOLD)
        now_danger = tf.greater(current_water, self.DANGER_THRESHOLD)
        changed_mask = tf.not_equal(was_danger, now_danger)

        # 2. Recorte (View Culling)
        map_shape = tf.shape(current_water)
        min_r, max_r, min_c, max_c = self.get_view_indices(view_min_x, view_max_x, view_min_y, view_max_y, map_shape)
        
        roi_changed = changed_mask[:, min_r:max_r, min_c:max_c, :]
        
        # 3. Extracción de coordenadas (Operación costosa, por eso la aislamos)
        local_indices = tf.where(roi_changed)
        
        # 4. Actualizar el "Snapshot" para la próxima vez
        # Solo actualizamos la región que estamos viendo o todo el mapa? 
        # Para consistencia simple, actualizamos todo el mapa, o usamos un buffer doble.
        # Opción segura: Assign global.
        self.v_last_rendered_water.assign(current_water)

        # 5. Procesar índices para retorno (Offset global)
        num_changes = tf.shape(local_indices)[0]

        def process_indices():
            l_rows = tf.cast(local_indices[:, 1], tf.int64)
            l_cols = tf.cast(local_indices[:, 2], tf.int64)
            g_rows = l_rows + tf.cast(min_r, tf.int64)
            g_cols = l_cols + tf.cast(min_c, tf.int64)
            return g_cols, g_rows

        def empty_result():
            return (tf.constant([], dtype=tf.int64), tf.constant([], dtype=tf.int64))
            
        out_x, out_y = tf.cond(num_changes > 0, process_indices, empty_result)

        return {
            "water_depth": tf.identity(current_water, name="out_water"),
            "changed_x": out_x,
            "changed_y": out_y
        }
