# -*- coding: utf-8 -*-
import tensorflow as tf
import os


# --- MODEL CONSTANTS ---
STATE_EMPTY = 0.0
STATE_STATIC = 1.0
STATE_DYNAMIC = 2.0

EPSILON_WATER = 1e-4  # Minimum height to consider that there is water
EPSILON_FLOW = 1e-6   # Minimum flow to consider that water is moving


def get_boundary_mask(shape, dy_offset, dx_offset):
    """
    Create a mask to override the wrap-around effect of tf.roll.
    Returns 0.0 in cells where the data comes from the opposite side of the map.
    """
    # shape is [batch, height, width, channels]
    h = shape[1]
    w = shape[2]

    # Mask for the Y axis (Rows)
    if dy_offset == -1:    # Movement to the North: the ‘wrap’ appears below, we cancel it
        m_y = tf.concat([tf.ones([h - 1]), [0.0]], axis=0)
    elif dy_offset == 1:   # Southbound movement: the ‘wrap’ appears above
        m_y = tf.concat([[0.0], tf.ones([h - 1])], axis=0)
    else:
        m_y = tf.ones([h])

    # Mask for the X axis (Columns)
    if dx_offset == -1: # Movement to the East: the ‘wrap’ appears on the right
        m_x = tf.concat([tf.ones([w - 1]), [0.0]], axis=0)
    elif dx_offset == 1:  # Movement to the West: the ‘wrap’ appears on the left
        m_x = tf.concat([[0.0], tf.ones([w - 1])], axis=0)
    else:
        m_x = tf.ones([w])

    # We construct the final mask [1, H, W, 1] using broadcasting.
    # We reshape so that TF can multiply the dimensions correctly.
    mask = tf.reshape(m_y, [1, h, 1, 1]) * tf.reshape(m_x, [1, 1, w, 1])

    return mask


@tf.function(input_signature=[
    tf.TensorSpec(shape=[1, None, None, 1], dtype=tf.float32, name="elevation"),
    tf.TensorSpec(shape=[1, None, None, 1], dtype=tf.float32, name="water_depth"),
    tf.TensorSpec(shape=[1, None, None, 1], dtype=tf.float32, name="roughness"),
    tf.TensorSpec(shape=[1, None, None, 1], dtype=tf.float32, name="cell_state"),
    tf.TensorSpec(shape=[], dtype=tf.float32, name="dt"),
    tf.TensorSpec(shape=[], dtype=tf.float32, name="dx")
])
def simulation_step(elevation, water_depth, roughness, cell_state, dt, dx):
    head = elevation + water_depth
    
    # Execution mask: Only DYNAMIC cells calculate output flows
    is_dynamic = tf.cast(tf.equal(cell_state, STATE_DYNAMIC), tf.float32)

    # 2. NEIGHBORHOOD CONFIGURATION (Moore 8)
    # (dy_offset, dx_offset, distance_factor)
    directions = [
        (1, 0, 1.0), (-1, 0, 1.0), (0, 1, 1.0), (0, -1, 1.0),          # N, S, E, O
        (-1, 1, 1.414), (-1, -1, 1.414), (1, 1, 1.414), (1, -1, 1.414) # Diagonals
    ]
    
    outflows = []
    
    # 3. PHASE I: CALCULATE OUTFLOWS (EDGE DRAINAGE)
    for dy_offset, dx_offset, distance_factor in directions:
        distance = distance_factor * dx
        
        # Obtain neighbor's level with Roll
        neighbor_head = tf.roll(head, shift=[dy_offset, dx_offset], axis=[1, 2])
        
        # ABSORBENT PADDING:
        # We multiply by the edge mask. If the cell is a boundary,
        # you will see ‘0.0’ outside the map, creating an output gradient (Drainage).
        b_mask = get_boundary_mask(tf.shape(water_depth), dy_offset, dx_offset)
        neighbor_head = neighbor_head * b_mask 
        
        # Hydraulic gradient (only if head_i > neighbor_head)
        gradient = tf.maximum(head - neighbor_head, 0.0)
        
        # Manning's equation: d_flow = (D^(5/3) * sqrt(grad) * dt) / (n * distance^1.5)
        denom = (roughness + 1e-7) * tf.pow(distance, 1.5)
        flow = (tf.pow(water_depth, 5.0/3.0) * tf.sqrt(gradient) * dt) / denom
        
        # Apply DYNAMIC mask (Only these cells process output)
        outflows.append(flow * is_dynamic)

    # 4. PHASE II: MASS CONSERVATION (LIMITER)
    total_potential_outflow = tf.add_n(outflows)
    # No more water_depth can come out than there is in the cell.
    limiter = tf.where(total_potential_outflow > 0, 
                       tf.minimum(1.0, water_depth / (total_potential_outflow + 1e-9)), 
                       1.0)

    # 5. PHASE III: CALCULATE NET FLOW (INFLOW - OUTFLOW)
    total_net_flow = tf.zeros_like(water_depth)
    
    for i, (dy_offset, dx_offset, _) in enumerate(directions):
        # Limited actual output
        real_out_f = outflows[i] * limiter
        
        # Entry from the neighbor (Reverse roll)
        in_f = tf.roll(real_out_f, shift=[-dy_offset, -dx_offset], axis=[1, 2])
        
        # ABSORBENT PADDING (Entry): 
        # We block the flow that ‘surrounds’ the map so that the water_depth does not reappear.
        in_mask = get_boundary_mask(tf.shape(water_depth), -dy_offset, -dx_offset)
        real_in_f = in_f * in_mask
        
        total_net_flow += (real_in_f - real_out_f)

    # 6. UPDATE STATUS
    output_water_depth = tf.maximum(water_depth + total_net_flow, 0.0)
    
    # 7. DETERMINE NEW CELL_STATE
    # If there is no water_depth -> EMPTY
    # If there is water_depth but it is not moving -> STATIC
    # If there is water_depth and significant flow -> DYNAMIC
    has_water_depth = tf.cast(tf.greater(output_water_depth, EPSILON_WATER), tf.float32)
    is_moving = tf.cast(tf.not_equal(total_net_flow, 0.0), tf.float32)
    
    output_cell_state = tf.where(has_water_depth > 0, 
                         tf.where(is_moving > 0, STATE_DYNAMIC, STATE_STATIC), 
                         STATE_EMPTY)


    # --- LÓGICA DE DETECCIÓN DE CAMBIOS ---
    
    # 1. Definir booleanos (Agua SI/NO)
    prev_had_water = tf.greater(cell_state, 0.0)
    curr_has_water = tf.greater(output_cell_state, 0.0)
    
    # 2. XOR: True si el estado cambió (se secó o se mojó)
    changed_mask = tf.not_equal(prev_had_water, curr_has_water)
    
    # 3. Obtener índices [N, 2] -> (fila/y, col/x)
    indices_chg = tf.where(changed_mask)
    
    # YA NO CALCULAMOS values_chg = tf.gather_nd(...) <--- ELIMINADO
    
    # 4. Transponer y Cast a INT32
    indices_T = tf.transpose(indices_chg)
    
    # Extraemos filas 1 (Y) y 2 (X) (Recordando que dim 0 es batch, dim 3 es canal)
    out_y_idx = tf.cast(indices_T[1], tf.int32)
    out_x_idx = tf.cast(indices_T[2], tf.int32)

    return tf.identity(output_water_depth, name="output_water_depth"), \
           tf.identity(output_cell_state, name="output_cell_state"), \
           tf.identity(out_x_idx, name="changed_x"), \
           tf.identity(out_y_idx, name="changed_y")


# --- EXPORT PROCESS ---
if __name__ == "__main__":
    print("Generating a TensorFlow graph for Cellular Automata...")
    
    # 1. Obtain the specific function
    full_model = simulation_step.get_concrete_function()
    
    # 2. Output directory
    output_dir = 'models'
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    
    # 3. Save as Frozen Graph (.pb)
    tf.io.write_graph(full_model.graph, output_dir, "simulation_model.pb", as_text=False)

    print(f"Success: Model exported in {output_dir}/simulation_model.pb")
