from __future__ import annotations

from abc import ABC, abstractmethod

import numpy as np


class DepthProvider(ABC):
    @abstractmethod
    def setup(self, rows: int, cols: int) -> None: ...

    @abstractmethod
    def get_water_depths(self, palette_grid: np.ndarray) -> np.ndarray: ...

    def apply_float_changes(self, changes: list[tuple[int, int, float]]) -> None:
        """No-op by default. Stateful providers override this."""
        pass
