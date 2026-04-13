
import tensorflow as tf
import math

class FloodModelCA(tf.Module):
    """Hydrodynamic Cellular Automata (m:n-CA^k) model for flood simulation.
    
    This module computes the propagation of shallow water flows over a 2D raster 
    grid using a multi-physics Kinematic Wave approximation and a Multiple Flow 
    Direction (MFD) routing algorithm. Designed to be compiled into a static graph 
    (ONNX) leveraging tensorial data structures.
    """

    def __init__(self):
        """Initializes the FloodModelCA."""
        super(FloodModelCA, self).__init__()

    @tf.function(input_signature=[
        tf.TensorSpec(shape=[None, None], dtype=tf.int8, name="land_cover")
    ])
    def preprocess(self, land_cover):
        """Precomputes static layers required for the simulation.
        
        Converts the discrete land cover categories into a continuous 
        surface roughness map (Manning's mu_soil) using a Lookup Table (LUT).

        Args:
            land_cover (tf.Tensor): A 2D tf.int8 tensor representing land use 
                categories.

        Returns:
            dict: A dictionary containing the precomputed roughness tensor (mu_soil).
        """
        default_manning = 0.050

        # LUT mapping land use categories to Manning's n (mu_soil)
        manning_lut = tf.constant([
            default_manning, # No data / Default (0.050)
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
        ], dtype=tf.float32, name="manning_lut")

        mu_soil = tf.gather(manning_lut, tf.cast(land_cover, tf.int32))

        return {"roughness:out": mu_soil}

    @tf.function(input_signature=[
        tf.TensorSpec(shape=[], dtype=tf.float32, name="delta_x"),
        tf.TensorSpec(shape=[], dtype=tf.float32, name="delta_t"),
        tf.TensorSpec(shape=[], dtype=tf.float32, name="fluid_density"),
        tf.TensorSpec(shape=[], dtype=tf.float32, name="fluid_viscosity"),
        tf.TensorSpec(shape=[None, None, None], dtype=tf.float32, name="water_depth"),
        tf.TensorSpec(shape=[None, None, None], dtype=tf.float32, name="topo_bathy"),
        tf.TensorSpec(shape=[None, None, None], dtype=tf.float32, name="roughness")
    ])
    def step(self, delta_x, delta_t, fluid_density, fluid_viscosity, water_depth, topo_bathy, roughness):
        """Executes a single time iteration (t -> t+1) of the model.

        Uses a multi-physics Kinematic Wave approach combined with explicit spatial 
        (Level Equalization) and temporal (Volumetric CFL) limiters to unconditionally 
        preserve volumetric continuity and avoid checkerboard instabilities.

        Args:
            delta_x (tf.Tensor): Spatial resolution of the grid (dx).
            delta_t (tf.Tensor): Time step duration (dt).
            fluid_density (tf.Tensor): Density of the fluid (rho).
            fluid_viscosity (tf.Tensor): Dynamic viscosity of the fluid (mu_fluid).
            water_depth (tf.Tensor): Current water depth (WD*).
            topo_bathy (tf.Tensor): Topobathymetry elevation layer.
            roughness (tf.Tensor): Surface roughness layer.

        Returns:
            dict: Updated water depth (WD^{t+1}) and automaton discrete cell states.
        """
        wall_height = tf.constant(9999.0, dtype=tf.float32, name="wall_height")
        sqrt_2 = tf.constant(math.sqrt(2.0), dtype=tf.float32, name="sqrt_2")
        g = tf.constant(9.81, dtype=tf.float32, name="gravity")

        # ====================================================================
        # PHASE 1: WSE AND HYDRAULIC GRADIENTS (S_max)
        # ====================================================================
        wse = topo_bathy + water_depth

        wse_padded = tf.pad(
            wse, 
            [[0, 0], [1, 1], [1, 1]], 
            mode='CONSTANT', 
            constant_values=wall_height
        )

        N  = wse_padded[:, 0:-2, 1:-1]
        S  = wse_padded[:, 2:,   1:-1]
        E  = wse_padded[:, 1:-1, 2:]
        W  = wse_padded[:, 1:-1, 0:-2]
        NE = wse_padded[:, 0:-2, 2:]
        NW = wse_padded[:, 0:-2, 0:-2]
        SE = wse_padded[:, 2:,   2:]
        SW = wse_padded[:, 2:,   0:-2]

        delta_wse = tf.stack([
            wse - N, wse - S, wse - E, wse - W,
            wse - NE, wse - NW, wse - SE, wse - SW
        ], axis=0)

        # Only evaluate flows towards neighbors with lower elevation
        delta_wse_positive = tf.maximum(delta_wse, 0.0)

        ortho_dist = delta_x
        diag_dist = delta_x * sqrt_2
        
        neighbor_distances = tf.reshape(
            tf.stack([ortho_dist, ortho_dist, ortho_dist, ortho_dist, 
                      diag_dist, diag_dist, diag_dist, diag_dist]), 
            [8, 1, 1, 1]
        )

        # Directional hydraulic gradient (S_k) and max local slope (S_max)
        s_k = delta_wse_positive / neighbor_distances
        s_max = tf.reduce_max(s_k, axis=0)
        sum_slopes = tf.reduce_sum(s_k, axis=0)
        
        # ====================================================================
        # PHASE 2: KINEMATICS, VISCOSITY AND PHYSICAL LIMITERS
        # ====================================================================
        cell_area = tf.square(delta_x)
        
        # 1. TURBULENT REGIME (Light fluids, pure water)
        # v_turb = (1 / roughness) * (WD^*)^(2/3) * sqrt(S_max)
        inv_roughness = tf.math.divide_no_nan(1.0, roughness)
        v_turb = inv_roughness * tf.pow(water_depth, 2.0/3.0) * tf.sqrt(s_max)
        
        # 2. LAMINAR / VISCOUS REGIME (Dense, non-Newtonian flows like mud)
        # v_lam = (rho * g * (WD^*)^2 * S_max) / (3 * mu_fluid)
        v_lam = (fluid_density * g * tf.square(water_depth) * s_max) / (3.0 * fluid_viscosity + 1e-8)
        
        # 3. EFFECTIVE FLUID VELOCITY (Multi-physics core)
        # Fluid travels at the lowest of the two velocities naturally bounding the flow.
        # v_approx = min(v_turb, v_lam)
        v_approx = tf.minimum(v_turb, v_lam)

        # Theoretical outflow volume: V_theor = v_approx * (WD^* * dx) * dt
        cross_section_area = water_depth * delta_x
        V_theor = v_approx * cross_section_area * delta_t

        # Distribute V_theor into directional outflow volumes using MFD weights
        flow_weights = tf.math.divide_no_nan(s_k, sum_slopes)
        delta_V_theor_k = V_theor * flow_weights

        # 4. SPATIAL SHIELD (Level Equalization)
        # Prevents hydraulic gradient inversion. Cell cannot drop water level 
        # beyond half the max elevation difference with its lowest neighbor.
        # delta_V_spatial = (max(delta_WSE) * dx^2) / 2
        max_delta_wse = tf.reduce_max(delta_wse_positive, axis=0)
        delta_V_spatial = (max_delta_wse * cell_area) / 2.0
        
        # 5. TEMPORAL SHIELD (Volumetric CFL Condition)
        # Effective volume is bounded by physical fluid mass and spatial limits.
        # V_avail = min(WD^* * dx^2, delta_V_spatial)
        V_cell = water_depth * cell_area
        V_avail = tf.minimum(V_cell, delta_V_spatial)
        
        total_outflow_attempt = tf.reduce_sum(delta_V_theor_k, axis=0)
        
        # Strict scale factor (alpha). Scales down flows proportionally if limits exceeded.
        # alpha = min(sum(delta_V_theor_k), V_avail) / sum(delta_V_theor_k)
        alpha = tf.math.divide_no_nan(
            tf.minimum(total_outflow_attempt, V_avail), 
            total_outflow_attempt
        )
        
        # Safe and volumetrically continuous directional outflows
        # delta_V_out_k = delta_V_theor_k * alpha
        delta_V_out_k = delta_V_theor_k * alpha

        # ====================================================================
        # PHASE 3: MFD ROUTING AND STATE UPDATE
        # ====================================================================
        total_outflow = tf.reduce_sum(delta_V_out_k, axis=0)
        
        outflow_padded = tf.pad(
            delta_V_out_k, 
            [[0, 0], [0, 0], [1, 1], [1, 1]], 
            mode='CONSTANT', 
            constant_values=0.0
        )

        # sum_{k in V} delta_V_{in <- k}
        total_inflow = (
            outflow_padded[0, :, 2:,   1:-1] + 
            outflow_padded[1, :, 0:-2, 1:-1] + 
            outflow_padded[2, :, 1:-1, 0:-2] + 
            outflow_padded[3, :, 1:-1, 2:]   + 
            outflow_padded[4, :, 2:,   0:-2] + 
            outflow_padded[5, :, 2:,   2:]   + 
            outflow_padded[6, :, 0:-2, 0:-2] + 
            outflow_padded[7, :, 0:-2, 2:]     
        )
        
        # WD^{t+1} = WD^* - Outflow + Inflow
        wd_next = water_depth - (total_outflow / cell_area) + (total_inflow / cell_area)
        
        # --- FLOATING-POINT PRECISION FILTER ---
        epsilon = 1e-5
        wd_next = tf.where(
            wd_next < epsilon, 
            0.0, 
            wd_next
        )

        # Cell States update for potential rendering or visualization
        has_water = wd_next > 0.001
        is_moving = (total_outflow > 1e-5) | (total_inflow > 1e-5)
        
        state_tensor = tf.where(has_water, 
                                tf.where(is_moving, tf.constant(2, dtype=tf.int8), tf.constant(1, dtype=tf.int8)), 
                                tf.constant(0, dtype=tf.int8))

        return {
            "water_depth:out": wd_next,
            "cell_state:out": state_tensor
        }
