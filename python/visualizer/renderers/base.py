from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import dataclass

import numpy as np


@dataclass(frozen=True)
class GridMeta:
    rows: int
    cols: int
    cell_size_m: float
    terrain_heights: np.ndarray | None  # float32, flat (rows*cols,)


@dataclass(frozen=True)
class FrameData:
    palette_grid: np.ndarray   # uint8, shape (rows, cols)
    water_depths: np.ndarray   # float32, shape (rows, cols)


class BaseRenderer(ABC):
    @abstractmethod
    def setup(self, meta: GridMeta) -> None: ...

    @abstractmethod
    def save_snapshot(self, frame: FrameData, step_index: int) -> None: ...

    @abstractmethod
    def close(self) -> None: ...
