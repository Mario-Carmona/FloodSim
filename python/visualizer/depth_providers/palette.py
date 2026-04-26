from __future__ import annotations

import numpy as np

from .base import DepthProvider

# Depth floor (meters) for each palette index.
# Must match flood_risk.levels[*].threshold_start in the scenario YAML.
_DEFAULT_THRESHOLDS: dict[int, float] = {
    0: 0.0,    # Dry
    1: 0.001,  # Very Shallow
    2: 0.1,    # Low Depth
    3: 0.3,    # Medium Depth
    4: 1.0,    # High Depth
    5: 2.0,    # Extreme Depth
}


class PaletteDepthProvider(DepthProvider):
    """Maps uint8 palette indices to approximate water depth floats."""

    def __init__(self) -> None:
        self._lookup: np.ndarray | None = None

    def setup(self, rows: int, cols: int) -> None:
        max_index = max(_DEFAULT_THRESHOLDS) + 1
        table = np.zeros(max_index, dtype=np.float32)
        for idx, depth in _DEFAULT_THRESHOLDS.items():
            table[idx] = depth
        self._lookup = table

    def get_water_depths(self, palette_grid: np.ndarray) -> np.ndarray:
        if self._lookup is None:
            raise RuntimeError("PaletteDepthProvider.setup() must be called before get_water_depths()")
        clamped = np.clip(palette_grid, 0, len(self._lookup) - 1)
        return self._lookup[clamped]
