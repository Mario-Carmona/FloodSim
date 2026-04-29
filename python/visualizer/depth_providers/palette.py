from __future__ import annotations

import json
import logging

import numpy as np

from .. import config
from .base import DepthProvider

_logger = logging.getLogger(__name__)


def _load_thresholds() -> dict[int, float]:
    """Load palette-index → depth (m) mapping from flood_levels.json."""
    try:
        with open(config.FLOOD_LEVELS_FILE, encoding="utf-8") as f:
            levels = json.load(f)
        return {entry["index"]: float(entry["threshold_start"]) for entry in levels}
    except Exception as exc:
        _logger.error("Failed to load flood levels from %s: %s", config.FLOOD_LEVELS_FILE, exc)
        raise


class PaletteDepthProvider(DepthProvider):
    """Maps uint8 palette indices to approximate water depth floats."""

    def __init__(self) -> None:
        self._lookup: np.ndarray | None = None

    def setup(self, rows: int, cols: int) -> None:
        thresholds = _load_thresholds()
        max_index = max(thresholds) + 1
        table = np.zeros(max_index, dtype=np.float32)
        for idx, depth in thresholds.items():
            table[idx] = depth
        self._lookup = table
        _logger.info("PaletteDepthProvider loaded %d levels from %s", len(thresholds), config.FLOOD_LEVELS_FILE)

    def get_water_depths(self, palette_grid: np.ndarray) -> np.ndarray:
        if self._lookup is None:
            raise RuntimeError("PaletteDepthProvider.setup() must be called before get_water_depths()")
        clamped = np.clip(palette_grid, 0, len(self._lookup) - 1)
        return self._lookup[clamped]
