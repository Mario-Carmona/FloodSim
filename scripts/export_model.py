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
    tf.TensorSpec(shape=[1, None, None, 4], dtype=tf.float32, name="input_state"),
    tf.TensorSpec(shape=[], dtype=tf.float32, name="dt"),
    tf.TensorSpec(shape=[], dtype=tf.float32, name="dx")
])
def simulation_step(input_state, dt, dx):
    # 1. UNPACK LAYERS
    # Channel 0: Elevation, 1: Water, 2: Manning n, 3: State
    elev       = input_state[:, :, :, 0:1]
    water      = input_state[:, :, :, 1:2]
    roughness  = input_state[:, :, :, 2:3]
    cell_state = input_state[:, :, :, 3:4]
    
    head = elev + water
    
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
        b_mask = get_boundary_mask(tf.shape(water), dy_offset, dx_offset)
        neighbor_head = neighbor_head * b_mask 
        
        # Hydraulic gradient (only if head_i > neighbor_head)
        gradient = tf.maximum(head - neighbor_head, 0.0)
        
        # Manning's equation: d_flow = (D^(5/3) * sqrt(grad) * dt) / (n * distance^1.5)
        denom = (roughness + 1e-7) * tf.pow(distance, 1.5)
        flow = (tf.pow(water, 5.0/3.0) * tf.sqrt(gradient) * dt) / denom
        
        # Apply DYNAMIC mask (Only these cells process output)
        outflows.append(flow * is_dynamic)

    # 4. PHASE II: MASS CONSERVATION (LIMITER)
    total_potential_outflow = tf.add_n(outflows)
    # No more water can come out than there is in the cell.
    limiter = tf.where(total_potential_outflow > 0, 
                       tf.minimum(1.0, water / (total_potential_outflow + 1e-9)), 
                       1.0)

    # 5. PHASE III: CALCULATE NET FLOW (INFLOW - OUTFLOW)
    total_net_flow = tf.zeros_like(water)
    
    for i, (dy_offset, dx_offset, _) in enumerate(directions):
        # Limited actual output
        real_out_f = outflows[i] * limiter
        
        # Entry from the neighbor (Reverse roll)
        in_f = tf.roll(real_out_f, shift=[-dy_offset, -dx_offset], axis=[1, 2])
        
        # ABSORBENT PADDING (Entry): 
        # We block the flow that ‘surrounds’ the map so that the water does not reappear.
        in_mask = get_boundary_mask(tf.shape(water), -dy_offset, -dx_offset)
        real_in_f = in_f * in_mask
        
        total_net_flow += (real_in_f - real_out_f)

    # 6. UPDATE STATUS
    new_water = tf.maximum(water + total_net_flow, 0.0)
    
    # 7. DETERMINE NEW CELL_STATE
    # If there is no water -> EMPTY
    # If there is water but it is not moving -> STATIC
    # If there is water and significant flow -> DYNAMIC
    has_water = tf.cast(tf.greater(new_water, EPSILON_WATER), tf.float32)
    is_moving = tf.cast(tf.not_equal(total_net_flow, 0.0), tf.float32)
    
    new_state = tf.where(has_water > 0, 
                         tf.where(is_moving > 0, STATE_DYNAMIC, STATE_STATIC), 
                         STATE_EMPTY)

    # RE-PACKAGING: We maintain constant elevation and roughness.
    output = tf.concat([elev, new_water, roughness, new_state], axis=-1)
    
    return tf.identity(output, name="output_state")


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
    # This file will be loaded by CoreEngineCA.cpp.
    tf.io.write_graph(full_model.graph.as_graph_def(), 
                      output_dir, 
                      'model_ca.pb', 
                      as_text=False)
    
    print(f"Success: Model exported in {output_dir}/model_ca.pb")
    print("Input nodes: input_state, dt, dx")
    print("Output node: output_state")
