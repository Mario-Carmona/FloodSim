from __future__ import annotations

import math
from dataclasses import dataclass

import numpy as np

from ..base import FrameData, GridMeta

_WATER_MIN_THRESHOLD = 0.0005  # metres — filter visual noise


@dataclass
class _TerrainStyle:
    diffuse: str
    shininess: float = 0.0
    transparency: float = 0.0


def _terrain_style(height_range: float) -> _TerrainStyle:
    if height_range < 0.5:
        return _TerrainStyle("0.62 0.50 0.25")   # sandy
    if height_range < 2.0:
        return _TerrainStyle("0.34 0.52 0.22")   # green
    return _TerrainStyle("0.60 0.60 0.60")        # rock


def _water_style() -> _TerrainStyle:
    return _TerrainStyle("0.05 0.45 0.85", shininess=0.8, transparency=0.3)


class X3DSerializer:
    """Pure string serialization of a simulation snapshot to an X3D/HTML document.

    No filesystem access — fully unit-testable.
    Call configure() once after init, then serialize() per frame.
    """

    def configure(self, meta: GridMeta) -> None:
        if meta.rows == 0 or meta.cols == 0:
            raise ValueError("X3DSerializer: rows and cols must be > 0")
        self._meta = meta
        self._configured = True

    def serialize(self, frame: FrameData) -> str:
        if not getattr(self, "_configured", False):
            raise RuntimeError("X3DSerializer.configure() must be called before serialize()")

        meta = self._meta
        rows, cols = meta.rows, meta.cols
        spacing = meta.cell_size_m

        terrain = self._get_terrain(rows, cols)
        water = self._get_water(frame.water_depths, rows, cols)

        grid_w = cols * spacing
        grid_h = rows * spacing
        viewpoint = self._compute_viewpoint(grid_w, grid_h)

        height_range = float(terrain.max() - terrain.min()) if terrain.size else 0.0
        ts = _terrain_style(height_range)
        ws = _water_style()

        terrain_str = self._heights_to_str(terrain)
        water_str = self._heights_to_str(water)

        scene = f"""<X3D width="100%" height="100%">
  <Scene>
    <Background skyColor="0.53 0.81 0.98"/>
    <NavigationInfo type='"EXAMINE" "ANY"'/>
    <Viewpoint position="{viewpoint}" orientation="1 0 0 -0.6" description="Overview"/>
    <DirectionalLight direction="0 -1 -0.5" intensity="0.8"/>
    <AmbientLight intensity="0.3"/>

    <!-- Terrain -->
    <Shape>
      <Appearance>
        <Material diffuseColor="{ts.diffuse}" shininess="{ts.shininess:.2f}" transparency="{ts.transparency:.2f}"/>
      </Appearance>
      <ElevationGrid
        xDimension="{cols}" zDimension="{rows}"
        xSpacing="{spacing:.4f}" zSpacing="{spacing:.4f}"
        height="{terrain_str}"
        solid="false"/>
    </Shape>

    <!-- Water -->
    <Shape>
      <Appearance>
        <Material diffuseColor="{ws.diffuse}" shininess="{ws.shininess:.2f}" transparency="{ws.transparency:.2f}"/>
      </Appearance>
      <ElevationGrid
        xDimension="{cols}" zDimension="{rows}"
        xSpacing="{spacing:.4f}" zSpacing="{spacing:.4f}"
        height="{water_str}"
        solid="false"/>
    </Shape>
  </Scene>
</X3D>"""

        return self._wrap_html(scene, meta)

    # ------------------------------------------------------------------
    # Private helpers
    # ------------------------------------------------------------------

    def _get_terrain(self, rows: int, cols: int) -> np.ndarray:
        meta = self._meta
        if meta.terrain_heights is not None and meta.terrain_heights.size == rows * cols:
            raw = meta.terrain_heights.reshape(-1).astype(np.float32)
        else:
            raw = np.zeros(rows * cols, dtype=np.float32)
        return self._sanitize(raw)

    def _get_water(self, water_depths: np.ndarray, rows: int, cols: int) -> np.ndarray:
        meta = self._meta
        terrain = self._get_terrain(rows, cols)
        flat = water_depths.flatten().astype(np.float32)
        flat = self._sanitize(flat)
        # Suppress visual noise below threshold
        flat = np.where(flat < _WATER_MIN_THRESHOLD, 0.0, flat)
        # Water surface sits on top of terrain
        result = terrain + flat
        # Where no water, collapse to terrain level to avoid artifacts
        result = np.where(flat == 0.0, terrain, result)
        return result

    def _sanitize(self, values: np.ndarray) -> np.ndarray:
        out = values.copy()
        out = np.where(np.isnan(out), 0.0, out)
        out = np.where(np.isposinf(out), 1000.0, out)
        out = np.where(np.isneginf(out), -1000.0, out)
        out = np.clip(out, -1000.0, 10000.0)
        return out

    def _heights_to_str(self, values: np.ndarray) -> str:
        return " ".join(f"{v:.4f}" for v in values)

    def _compute_viewpoint(self, grid_w: float, grid_h: float) -> str:
        cx = grid_w * 0.5
        cz = grid_h * 0.5
        max_dim = max(grid_w, grid_h)
        cam_x = cx + max_dim * 0.75
        cam_y = max_dim * 0.8
        cam_z = cz + max_dim * 1.05
        return f"{cam_x:.2f} {cam_y:.2f} {cam_z:.2f}"

    def _wrap_html(self, scene: str, meta: GridMeta) -> str:
        return f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8"/>
  <title>DanaSim — {meta.rows}x{meta.cols} grid</title>
  <script src="https://cdn.jsdelivr.net/npm/x_ite@latest/dist/x_ite.min.js"></script>
  <style>
    html, body {{ margin: 0; height: 100%; background: #1a1a2e; }}
    x3d-canvas {{ width: 100vw; height: 100vh; display: block; }}
  </style>
</head>
<body>
  <!-- rows={meta.rows} cols={meta.cols} cell_size={meta.cell_size_m:.4f}m -->
{scene}
</body>
</html>
"""
