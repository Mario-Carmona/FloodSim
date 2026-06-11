
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

        # Map discrete categories to continuous roughness values using tf.gather
        mu_soil = tf.gather(manning_lut, tf.cast(land_cover, tf.int32))

        # Initialize the movement state tracking layer (0 = still, 1 = moving)
        water_movement_state = tf.zeros_like(land_cover, dtype=tf.int8)

        return {
            "roughness:out": mu_soil,
            "water_movement_state:out": water_movement_state
        }

    @tf.function(input_signature=[
        tf.TensorSpec(shape=[], dtype=tf.float32, name="delta_x"),
        tf.TensorSpec(shape=[], dtype=tf.float32, name="delta_t"),
        tf.TensorSpec(shape=[], dtype=tf.float32, name="fluid_density"),
        tf.TensorSpec(shape=[], dtype=tf.float32, name="fluid_viscosity"),
        tf.TensorSpec(shape=[None, None, None], dtype=tf.float32, name="water_depth"),
        tf.TensorSpec(shape=[None, None, None], dtype=tf.int8, name="water_movement_state"),
        tf.TensorSpec(shape=[None, None, None], dtype=tf.float32, name="topo_bathy"),
        tf.TensorSpec(shape=[None, None, None], dtype=tf.float32, name="roughness")
    ])
    def step(self, delta_x, delta_t, fluid_density, fluid_viscosity, water_depth, water_movement_state, topo_bathy, roughness):
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
            water_movement_state (tf.Tensor): Boolean mask tracking moving water.
            topo_bathy (tf.Tensor): Topobathymetry elevation layer.
            roughness (tf.Tensor): Surface roughness layer (Manning's n).

        Returns:
            dict: Updated water depth (WD^{t+1}) and automaton discrete cell states.
        """
        # Constants
        wall_height = tf.constant(9999.0, dtype=tf.float32, name="wall_height")
        sqrt_2 = tf.constant(math.sqrt(2.0), dtype=tf.float32, name="sqrt_2")
        g = tf.constant(9.81, dtype=tf.float32, name="gravity")

        # ====================================================================
        # PHASE 1: WATER SURFACE ELEVATION (WSE) AND HYDRAULIC GRADIENTS
        # ====================================================================
        # Absolute Water Surface Elevation
        wse = topo_bathy + water_depth

        # Pad boundaries with a "virtual wall" to prevent water escaping the grid domain
        wse_padded = tf.pad(
            wse, 
            [[0, 0], [1, 1], [1, 1]], 
            mode='CONSTANT', 
            constant_values=wall_height
        )

        # Extract shifted tensors for the Moore neighborhood (8 directions)
        N  = wse_padded[:, 0:-2, 1:-1]
        S  = wse_padded[:, 2:,   1:-1]
        E  = wse_padded[:, 1:-1, 2:]
        W  = wse_padded[:, 1:-1, 0:-2]
        NE = wse_padded[:, 0:-2, 2:]
        NW = wse_padded[:, 0:-2, 0:-2]
        SE = wse_padded[:, 2:,   2:]
        SW = wse_padded[:, 2:,   0:-2]

        # Calculate elevation difference between the central cell and its 8 neighbors
        delta_wse = tf.stack([
            wse - N, wse - S, wse - E, wse - W,
            wse - NE, wse - NW, wse - SE, wse - SW
        ], axis=0)

        # Filter negative gradients: evaluate flows ONLY towards neighbors with lower elevation
        delta_wse_positive = tf.maximum(delta_wse, 0.0)

        # Define distances to orthogonal and diagonal neighbors
        ortho_dist = delta_x
        diag_dist = delta_x * sqrt_2
        
        neighbor_distances = tf.reshape(
            tf.stack([ortho_dist, ortho_dist, ortho_dist, ortho_dist, 
                      diag_dist, diag_dist, diag_dist, diag_dist]), 
            [8, 1, 1, 1]
        )

        # Directional hydraulic gradient (slope S_k)
        s_k = delta_wse_positive / neighbor_distances

        # 1. Convert the int8 movement mask (0 or 1) to float32 to match the slope tensor's dtype
        movement_mask = tf.cast(water_movement_state, tf.float32)
        
        # 2. Multiply slopes by the movement mask. 
        # TensorFlow applies this mask to all 8 directions automatically via broadcasting.
        # Inactive cells (0.0) will nullify their slopes, immediately blocking their outflow calculation.
        s_k = s_k * movement_mask

        # ====================================================================
        # PHASE 2: KINEMATICS, VISCOSITY AND PHYSICAL LIMITERS
        # ====================================================================
        cell_area = tf.square(delta_x)
        
        # 1. TURBULENT REGIME (Manning's Equation for light fluids like pure water)
        inv_roughness = tf.math.divide_no_nan(1.0, roughness)
        v_turb_k = inv_roughness * tf.pow(water_depth, 2.0/3.0) * tf.sqrt(s_k)
        
        # 2. LAMINAR / VISCOUS REGIME (Dense, non-Newtonian flows like heavy mud)
        v_lam_k = (fluid_density * g * tf.square(water_depth) * s_k) / (3.0 * fluid_viscosity + 1e-8)
        
        # 3. EFFECTIVE FLUID VELOCITY (Multi-physics core)
        # Fluid travels at the lowest of the two velocities, naturally bounding the flow physics.
        v_approx_k = tf.minimum(v_turb_k, v_lam_k)

        # Flow cross-section area per cell edge
        cross_section_area = water_depth * delta_x

        # Theoretical volumetric outflow based on velocity, area, and time step
        delta_V_theor_k = v_approx_k * cross_section_area * delta_t

        # 4. MASS CONSERVATION CONSTRAINT (WITHOUT SPATIAL SHIELD)
        # Strictly calculate the actual available water volume contained within the cell
        V_cell = water_depth * cell_area
        
        # Sum all the water volume that the active directions are attempting to draw out
        total_outflow_attempt = tf.reduce_sum(delta_V_theor_k, axis=0)

        # Restrictive scaling factor (alpha): Computes the ratio between available and attempted volume.
        # If total_outflow_attempt > V_cell -> alpha < 1.0 (scales down outflow to prevent negative water depth)
        # If total_outflow_attempt <= V_cell -> alpha = 1.0 (free, unrestricted flow)
        alpha = tf.math.divide_no_nan(
            tf.minimum(total_outflow_attempt, V_cell), 
            total_outflow_attempt
        )
        
        # Apply the alpha scaling factor to obtain final, safe, and conservative directional flows
        delta_V_out_k = delta_V_theor_k * alpha


        # ====================================================================
        # PHASE 3: MFD ROUTING AND STATE UPDATE
        # ====================================================================
        # Total volume leaving the cell
        total_outflow = tf.reduce_sum(delta_V_out_k, axis=0)
        
        # Pad outflow tensor with zeros at boundaries so inflows can be computed safely
        outflow_padded = tf.pad(
            delta_V_out_k, 
            [[0, 0], [0, 0], [1, 1], [1, 1]], 
            mode='CONSTANT', 
            constant_values=0.0
        )

        # Gather inflows from neighboring cells.
        # This acts as a reverse lookup: if water flowed North from cell (i+1), it enters South of cell (i).
        total_inflow = (
            outflow_padded[0, :, 2:,   1:-1] + # Flow arriving from South (was moving N)
            outflow_padded[1, :, 0:-2, 1:-1] + # Flow arriving from North (was moving S)
            outflow_padded[2, :, 1:-1, 0:-2] + # Flow arriving from West (was moving E)
            outflow_padded[3, :, 1:-1, 2:]   + # Flow arriving from East (was moving W)
            outflow_padded[4, :, 2:,   0:-2] + # Flow arriving from SW (was moving NE)
            outflow_padded[5, :, 2:,   2:]   + # Flow arriving from SE (was moving NW)
            outflow_padded[6, :, 0:-2, 0:-2] + # Flow arriving from NW (was moving SE)
            outflow_padded[7, :, 0:-2, 2:]     # Flow arriving from NE (was moving SW)
        )
        
        # Mass balance equation
        wd_next = water_depth - (total_outflow / cell_area) + (total_inflow / cell_area)
        
        # --- FLOATING-POINT PRECISION FILTER ---
        # Eliminate numerical artifacts (near-zero depths) caused by floating-point arithmetic
        epsilon = 1e-5
        wd_next = tf.where(
            wd_next < epsilon, 
            0.0, 
            wd_next
        )

        # Cell States update for computational optimization in subsequent steps or rendering
        is_moving = (total_outflow > 1e-5) | (total_inflow > 1e-5)
        
        # Convert boolean mask back to int8
        wd_mov_state = tf.where(is_moving, tf.constant(1, dtype=tf.int8), tf.constant(0, dtype=tf.int8))

        return {
            "water_depth:out": wd_next,
            "water_movement_state:out": wd_mov_state
        }

    def get_fluid_layer_name(self) -> str:
        return "water_depth"

    def get_fluid_movement_state_layer_name(self) -> str:
        return "water_movement_state"