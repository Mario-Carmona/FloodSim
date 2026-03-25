
import tensorflow as tf


class SimulatorModule(tf.Module):
    def __init__(self):
        super().__init__()
        # Lookup Table (LUT) converted into a 1D Tensor
        # Position = ID, Value = Manning coefficient
        self.DEFAULT_MANNING = 0.050

        self.MANNING_LUT = [
            self.DEFAULT_MANNING, # No data / Default (0.050)
            0.080, # "Urbano mixto" (Edificaciones, asfalto y algo de verde)
            0.120, # "Casco" (Alta densidad, flujo muy bloqueado por edificios)
            0.100, # "Ensanche" (Bloques regulares, calles anchas)
            0.060, # "Discontinuo" (Casas aisladas, jardines)
            0.050, # "Zonas verdes urbanas" (Parques, césped mantenido)
            0.040, # "Instalaciones del sector primario" (Silos, naves despejadas)
            0.040, # "Instalaciones agrícolas y/o ganaderas"
            0.050, # "Instalaciones forestales"
            0.040, # "Extracción minera" (Tierra removida, grava, roca)
            0.060, # "Industrial" (Naves grandes, mucho asfalto/hormigón)
            0.060, # "Servicios y dotacional" (Hospitales, colegios, asfalto y patios)
            0.045, # "Asentamiento agrícola y huertas" (Vegetación dispersa y construcciones menores)
            0.020, # "Infraestructura de transporte" (Asfalto/hormigón limpio, peajes)
            0.015, # "Redes viarias y ferroviarias" (Carreteras asfaltadas lisas, balasto)
            0.030, # "Puertos" (Muelles de hormigón lisos pero con obstáculos)
            0.020, # "Aeropuertos" (Pistas de aterrizaje largas, muy lisas)
            0.050, # "Infraestructuras técnicas" (Depuradoras, subestaciones)
            0.040, # "Infraestructuras de suministro"
            0.050, # "Infraestructuras de residuos" (Vertederos, terreno irregular)
            0.035, # "Cultivos herbáceos" (Trigo, cebada, pasto corto)
            0.025, # "Invernaderos" (Cubiertas de plástico lisas que aceleran el agua)
            0.050, # "Cultivos leñosos" (Árboles alineados, suelo arado)
            0.045, # "Frutales cítricos"
            0.045, # "Frutales no cítricos"
            0.040, # "Viñedo" (Hileras regulares, suelo a menudo desnudo)
            0.045, # "Olivares"
            0.050, # "Otros cultivos leñosos"
            0.055, # "Combinaciones de cultivos leñosos"
            0.035, # "Prado" (Hierba densa, pastos)
            0.040, # "Combinaciones de cultivos"
            0.045, # "Combinaciones de cultivos con vegetación"
            0.100, # "Bosque" (Alta fricción: troncos, ramas secas, maleza)
            0.110, # "Bosque de frondosas" (Suelo muy cubierto de hojas y ramas gruesas)
            0.120, # "Bosque de coníferas" (Gran acumulación de agujas y sotobosque espeso)
            0.115, # "Bosque mixto"
            0.035, # "Pastizal o herbazal" (Hierba natural)
            0.070, # "Matorral" (Arbustos densos, zarzas que frenan el agua)
            0.080, # "Combinaciones de vegetación" (Matorral y pasto mezclado)
            0.025, # "Terrenos sin vegetación" (Tierra compactada, tierra lisa)
            0.015, # "Playas, dunas y arenales" (Arena lisa, aunque muy permeable, superficialmente lisa)
            0.035, # "Roquedo" (Piedra irregular, cantos rodados)
            0.060, # "Temporalmente desarbolado por incendios" (Restos quemados, ramas caídas, suelo inestable)
            0.025, # "Suelo desnudo" (Tierra descubierta o arcilla)
            0.040, # "Coberturas húmedas" (Zonas encharcadas con vegetación baja)
            0.050, # "Zonas húmedas y pantanosas" (Cañas, juncos gruesos)
            0.060, # "Turberas" (Suelo muy esponjoso, musgo profundo)
            0.045, # "Marismas" (Suelo salino con vegetación halófila)
            0.020, # "Salinas" (Superficies de evaporación muy planas y lisas)
            0.025, # "Coberturas de agua" (Agua superficial con algo de fricción en el fondo)
            0.030, # "Cursos de agua" (Ríos naturales, lechos irregulares con piedras)
            0.020, # "Lagos y lagunas" (Superficies amplias, poca fricción relativa al fondo)
            0.020, # "Embalses" (Aguas profundas, fondo lejano)
            0.015, # "Láminas de agua artificial" (Canales de hormigón, piscinas)
            0.020, # "Mares y océanos"
            0.010  # "Glaciares y nieves perpetuas" (Hielo sólido y liso, fricción mínima)
        ]

    @tf.function(input_signature=[
        tf.TensorSpec(shape=[1, None, None, 1], dtype=tf.int8, name="land_cover"),
        tf.TensorSpec(shape=[], dtype=tf.float32, name="dt"),
        tf.TensorSpec(shape=[], dtype=tf.float32, name="dx")
    ])
    def precompute(self, land_cover, dt, dx):
        # Mapea los IDs a valores de fricción/conductancia de Manning
        lc_indices = tf.cast(land_cover, tf.int32)
        manning_grid = tf.gather(self.MANNING_LUT, lc_indices)

        k_spatial = manning_grid * (dt / (dx * dx))

        return {"k_spatial": k_spatial}

    @tf.function(input_signature=[
        tf.TensorSpec(shape=[1, None, None, 1], dtype=tf.float32, name="water_depth"),
        tf.TensorSpec(shape=[1, None, None, 1], dtype=tf.float32, name="topo_bathy"),
        tf.TensorSpec(shape=[1, None, None, 1], dtype=tf.float32, name="k_spatial"),
        tf.TensorSpec(shape=[1, None, None, 1], dtype=tf.float32, name="rainfall")
    ])
    def run(self, water_depth, topo_bathy, k_spatial, rainfall):
        # 1. Añadir lluvia y calcular altura total
        water_depth = water_depth + rainfall
        head = topo_bathy + water_depth

        # -----------------------------------------------------------------
        # 2. DIFERENCIAS INTERNAS (Sin Padding Simétrico)
        # -----------------------------------------------------------------
        # Eje Y (Norte-Sur): Restamos la fila de abajo a la fila actual
        # diff_y > 0 significa que el agua fluye hacia el Sur
        diff_y = head[:, :-1, :, :] - head[:, 1:, :, :]
        
        diff_S_inner = tf.maximum(0.0, diff_y)  # Flujo interno hacia el Sur
        diff_N_inner = tf.maximum(0.0, -diff_y) # Flujo interno hacia el Norte

        # Eje X (Oeste-Este): Restamos la columna derecha a la columna actual
        # diff_x > 0 significa que el agua fluye hacia el Este
        diff_x = head[:, :, :-1, :] - head[:, :, 1:, :]
        
        diff_E_inner = tf.maximum(0.0, diff_x)  # Flujo interno hacia el Este
        diff_W_inner = tf.maximum(0.0, -diff_x) # Flujo interno hacia el Oeste

        # -----------------------------------------------------------------
        # 3. RECONSTRUIR MAPAS CON MUROS (Zero-Padding)
        # -----------------------------------------------------------------
        # Añadimos un 0 en el borde correspondiente para bloquear la salida
        diff_S = tf.pad(diff_S_inner, [[0,0], [0,1], [0,0], [0,0]]) # 0 al fondo
        diff_N = tf.pad(diff_N_inner, [[0,0], [1,0], [0,0], [0,0]]) # 0 arriba
        diff_E = tf.pad(diff_E_inner, [[0,0], [0,0], [0,1], [0,0]]) # 0 a la derecha
        diff_W = tf.pad(diff_W_inner, [[0,0], [0,0], [1,0], [0,0]]) # 0 a la izquierda

        total_diff = diff_N + diff_S + diff_W + diff_E

        # 4. Calcular Salidas
        requested_outflow = total_diff * k_spatial

        scaling = tf.where(requested_outflow > water_depth,
                           water_depth / (requested_outflow + 1e-8),
                           tf.ones_like(water_depth))

        out_N = diff_N * k_spatial * scaling
        out_S = diff_S * k_spatial * scaling
        out_W = diff_W * k_spatial * scaling
        out_E = diff_E * k_spatial * scaling

        total_out = out_N + out_S + out_W + out_E

        # -----------------------------------------------------------------
        # 5. CALCULAR ENTRADAS MEDIANTE DESPLAZAMIENTOS
        # -----------------------------------------------------------------
        # Lo que sale por el Sur de la celda de arriba, entra por el Norte de esta.
        # Desplazamos los mapas de salida cortando el último elemento y añadiendo un 0 al inicio.
        in_from_N = tf.pad(out_S[:, :-1, :, :], [[0,0], [1,0], [0,0], [0,0]])
        in_from_S = tf.pad(out_N[:, 1:, :, :],  [[0,0], [0,1], [0,0], [0,0]])
        in_from_W = tf.pad(out_E[:, :, :-1, :], [[0,0], [0,0], [1,0], [0,0]])
        in_from_E = tf.pad(out_W[:, :, 1:, :],  [[0,0], [0,0], [0,1], [0,0]])

        total_in = in_from_N + in_from_S + in_from_W + in_from_E

        # 6. Actualizar mapa final
        new_water_depth = water_depth - total_out + total_in
        
        return {"new_water_depth": new_water_depth} 
