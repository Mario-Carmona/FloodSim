
import numpy as np
import logging
import os
import sys

if __package__ in (None, ""):
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import config
else:
    from . import config

class SimulationGrid:
    """
    Manages the state of the simulation grid.
    """

    def __init__(self):
        # Store palette levels as uint8 values.
        self.grid = np.zeros((config.MAP_SIZE, config.MAP_SIZE), dtype=np.uint8)
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

    def update_from_layer_event(self, event: dict):
        """Apply layer changes from a JSON EYE_SetState_Layer event.

        Supported shape:
        {
          "changes": {
            "cells": [
              {"x": 1, "y": 2, "state": "FLOODED"},
              ...
            ]
          }
        }
        """
        changes = event.get("changes", {})
        cells = changes.get("cells", [])

        # Some producers may use a dict keyed by id. Convert to value list.
        if isinstance(cells, dict):
            cells = list(cells.values())

        if not isinstance(cells, list) or not cells:
            return

        rows = []
        cols = []
        values = []
        for cell in cells:
            x = cell.get("x")
            y = cell.get("y")
            if x is None or y is None:
                continue

            cell_value = self._resolve_cell_value(cell)
            if cell_value is None:
                continue

            cols.append(int(x))
            rows.append(int(y))
            values.append(int(cell_value))

        if not rows:
            return

        cols_arr = np.array(cols)
        rows_arr = np.array(rows)

        if np.any(cols_arr >= config.MAP_SIZE) or np.any(rows_arr >= config.MAP_SIZE):
            self._logger.error(
                "Coordinates out of bounds in layer event. Max X: %s, Max Y: %s",
                int(np.max(cols_arr)),
                int(np.max(rows_arr)),
            )
            return

        if np.any(cols_arr < 0) or np.any(rows_arr < 0):
            self._logger.error("Negative coordinates received in layer event.")
            return

        value_arr = np.array(values, dtype=np.uint8)
        self.grid[rows_arr, cols_arr] = value_arr
        self.has_new_data = True

    def _resolve_cell_value(self, cell: dict):
        """Resolve a numeric palette value from different cell payload variants."""
        for numeric_key in ("value", "level", "depth_level", "risk_level"):
            if numeric_key in cell:
                try:
                    return int(cell[numeric_key])
                except (TypeError, ValueError):
                    return None

        state = cell.get("state")
        if isinstance(state, str):
            return config.STATE_TO_VALUE.get(state.upper(), 1)

        return 1
