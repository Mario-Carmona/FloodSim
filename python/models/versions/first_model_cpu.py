
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
            0.080, # "Mixed urban" (Buildings, asphalt, and some greenery)
            0.120, # "Urban core / Old town" (High density, flow heavily blocked by buildings)
            0.100, # "Urban grid / Extension" (Regular blocks, wide streets)
            0.060, # "Discontinuous urban" (Isolated houses, gardens)
            0.050, # "Urban green zones" (Parks, maintained lawns)
            0.040, # "Primary sector facilities" (Silos, clear warehouses/sheds)
            0.040, # "Agricultural and/or livestock facilities"
            0.050, # "Forestry facilities"
            0.040, # "Mining extraction" (Excavated earth, gravel, rock)
            0.060, # "Industrial" (Large warehouses, lots of asphalt/concrete)
            0.060, # "Services and institutional" (Hospitals, schools, asphalt, and courtyards)
            0.045, # "Agricultural settlement and orchards" (Scattered vegetation and minor structures)
            0.020, # "Transportation infrastructure" (Clean asphalt/concrete, toll booths)
            0.015, # "Road and rail networks" (Smooth asphalt roads, ballast)
            0.030, # "Ports" (Smooth concrete docks but with obstacles)
            0.020, # "Airports" (Long runways, very smooth)
            0.050, # "Technical infrastructure" (Water treatment plants, substations)
            0.040, # "Supply infrastructure"
            0.050, # "Waste infrastructure" (Landfills, irregular terrain)
            0.035, # "Herbaceous crops" (Wheat, barley, short grass)
            0.025, # "Greenhouses" (Smooth plastic covers that accelerate water)
            0.050, # "Woody crops" (Lined trees, plowed soil)
            0.045, # "Citrus orchards"
            0.045, # "Non-citrus orchards"
            0.040, # "Vineyards" (Regular rows, often bare soil)
            0.045, # "Olive groves"
            0.050, # "Other woody crops"
            0.055, # "Woody crop combinations"
            0.035, # "Meadow" (Dense grass, pastures)
            0.040, # "Crop combinations"
            0.045, # "Crop combinations with vegetation"
            0.100, # "Forest" (High friction: trunks, dry branches, undergrowth)
            0.110, # "Broadleaf forest" (Floor heavily covered with leaves and thick branches)
            0.120, # "Coniferous forest" (Large accumulation of needles and thick understory)
            0.115, # "Mixed forest"
            0.035, # "Grassland or herbland" (Natural grass)
            0.070, # "Shrubland / Scrub" (Dense shrubs, brambles slowing down water)
            0.080, # "Vegetation combinations" (Mixed scrub and pasture)
            0.025, # "Unvegetated land" (Compacted earth, smooth soil)
            0.015, # "Beaches, dunes, and sandbanks" (Smooth sand, though highly permeable, superficially smooth)
            0.035, # "Rocky terrain" (Irregular stone, boulders/cobbles)
            0.060, # "Temporarily deforested by fires" (Burnt debris, fallen branches, unstable soil)
            0.025, # "Bare soil" (Exposed earth or clay)
            0.040, # "Wet cover" (Waterlogged areas with low vegetation)
            0.050, # "Wetlands and swamps" (Reeds, thick rushes)
            0.060, # "Peatlands / Bogs" (Very spongy soil, deep moss)
            0.045, # "Salt marshes" (Saline soil with halophytic vegetation)
            0.020, # "Salt flats / Salt pans" (Very flat and smooth evaporation surfaces)
            0.025, # "Water bodies / Water cover" (Surface water with some bottom friction)
            0.030, # "Watercourses" (Natural rivers, irregular stony beds)
            0.020, # "Lakes and lagoons" (Wide surfaces, low friction relative to the bottom)
            0.020, # "Reservoirs" (Deep water, distant bottom)
            0.015, # "Artificial water surfaces" (Concrete channels, pools)
            0.020, # "Seas and oceans"
            0.010  # "Glaciers and perpetual snow" (Solid and smooth ice, minimal friction)
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
        is_moving = (total_outflow > 1e-5) | (total_inflow > 1e-5)
        
        wd_mov_state = tf.where(is_moving, tf.constant(1, dtype=tf.int8), tf.constant(0, dtype=tf.int8))

        return {
            "water_depth:out": wd_next,
            "water_movement_state:out": wd_mov_state
        }

    def get_fluid_layer_name(self) -> str:
        return "water_depth"

    def get_fluid_movement_state_layer_name(self) -> str:
        return "water_movement_state"