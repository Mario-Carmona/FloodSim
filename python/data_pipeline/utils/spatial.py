from typing import List

import numpy as np


def calculate_grid_edges(
    min_val: float, 
    max_val: float, 
    step_size: float
) -> List[float]:
    """Calculates 1D spatial boundaries to divide a spatial extent into tiles.

    This function generates a list of coordinates starting from `min_val` and 
    incrementing by `step_size`. It guarantees that the final bounding edge 
    always perfectly covers `max_val`, even if the step size does not divide 
    the extent evenly.

    Args:
        min_val (float): The starting coordinate (minimum boundary) of the extent.
        max_val (float): The ending coordinate (maximum boundary) of the extent.
        step_size (float): The size of each tile/grid step.

    Returns:
        List[float]: A list of float values representing the edges of the grid.

    Raises:
        ValueError: If `step_size` is less than or equal to zero, or if 
            `min_val` is greater than or equal to `max_val`.
    """
    # 1. Input validation to prevent invalid grid generation
    if step_size <= 0:
        raise ValueError(f"step_size must be strictly positive, got {step_size}.")
    
    if min_val >= max_val:
        raise ValueError(
            f"min_val ({min_val}) must be strictly less than max_val ({max_val})."
        )

    # 2. Generate the base sequence of edges
    # np.arange creates values in the half-open interval [min_val, max_val)
    edges = np.arange(min_val, max_val, step_size).tolist()
    
    # 3. Ensure the maximum extent is always completely covered.
    # If the generated sequence is empty (rare edge case) or the last calculated 
    # edge falls short of the maximum boundary, append the exact max_val.
    if not edges or edges[-1] < max_val:
        edges.append(max_val)
        
    return edges