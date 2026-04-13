
import numpy as np
import logging
import mqtt_scripts.config

class SimulationGrid:
    """
    Manages the state of the simulation grid.
    """

    def __init__(self):
        # Initialize the grid as a 20x20 matrix of int8 (0 or 1).
        # Optimized for memory usage (int8 takes 1 byte per cell).
        self.grid = np.zeros((config.MAP_SIZE, config.MAP_SIZE), dtype=np.int8)
        self.has_new_data = False
        self._logger = logging.getLogger(__name__)

    def update_from_deltas(self, x_coords: list, y_coords: list, step_index: int):
        """
        Updates the grid state based on the received deltas.
        Applies XOR logic to invert the state of specific cells.
        """
        if not x_coords:
            if config.DEBUG_MODE:
                self._logger.debug(f"Frame {step_index}: No changes detected.")
            return

        cols = np.array(x_coords)
        rows = np.array(y_coords)

        # --- Validation Logic ---
        # Checks for out-of-bounds coordinates to prevent silent crashes.
        # This preserves the safety checks from the original script.
        if np.any(cols >= config.MAP_SIZE) or np.any(rows >= config.MAP_SIZE):
            self._logger.error(
                f"FATAL ERROR: Coordinates out of bounds in step {step_index}. "
                f"Max X: {np.max(cols)}, Max Y: {np.max(rows)}"
            )
            return

        if np.any(cols < 0) or np.any(rows < 0):
            self._logger.error("FATAL ERROR: Negative coordinates received.")
            return

        # --- Update Logic ---
        # Applies XOR operation (dry <-> wet) using NumPy indexing.
        # grid[rows, cols] ^= 1 handles the state toggle efficiently.
        self.grid[rows, cols] ^= 1
        
        self.has_new_data = True
        
        if config.DEBUG_MODE:
            self._logger.info(f"Frame {step_index} processed. {len(cols)} cells updated.")

    def consume_data(self):
        """
        Returns the current grid and resets the 'new data' flag.
        Useful for the visualizer to know if it needs to redraw.
        """
        self.has_new_data = False
        return self.grid
