from __future__ import annotations

import math
from dataclasses import dataclass

import numpy as np

from ..base import FrameData, GridMeta
from .colors import X3DColorScheme

_WATER_MIN_THRESHOLD = 0.0005  # metres — filter visual noise


@dataclass
class _TerrainStyle:
    diffuse: str
    shininess: float = 0.0
    transparency: float = 0.0


class X3DSerializer:
    """Pure string serialization of a simulation snapshot to an X3D/HTML document.

    No filesystem access — fully unit-testable.
    Call configure() once after init, then serialize() per frame.
    """

    def configure(self, meta: GridMeta, colors: X3DColorScheme | None = None) -> None:
        if meta.rows == 0 or meta.cols == 0:
            raise ValueError("X3DSerializer: rows and cols must be > 0")
        self._meta = meta
        self._colors = colors or X3DColorScheme()
        self._configured = True

    def serialize(self, frame: FrameData) -> tuple[str, str]:
        """Return (html_document, water_heights_str) for this frame.

        The water_heights_str is kept separate so the renderer can
        accumulate it for the animation player.
        """
        if not getattr(self, "_configured", False):
            raise RuntimeError("X3DSerializer.configure() must be called before serialize()")

        meta = self._meta
        rows, cols = meta.rows, meta.cols
        spacing = meta.cell_size_m

        terrain = self._get_terrain(rows, cols)
        water = self._get_water(frame.water_depths, rows, cols)

        grid_w = cols * spacing
        grid_h = rows * spacing
        max_height = float(terrain.max()) if terrain.size else 0.0
        viewpoint, orientation = self._compute_viewpoint(grid_w, grid_h, max_height)

        height_range = float(terrain.max() - terrain.min()) if terrain.size else 0.0
        ts = self._terrain_style(height_range)
        ws = self._water_style()

        terrain_str = self._heights_to_str(terrain)
        water_str = self._heights_to_str(water)

        scene = self._build_scene(cols, rows, spacing, viewpoint, orientation, ts, ws, terrain_str, water_str)
        return self._wrap_html(scene, meta), water_str

    def generate_player(self, water_frames: list[str]) -> str:
        """Generate a single HTML player that animates all frames."""
        if not getattr(self, "_configured", False):
            raise RuntimeError("X3DSerializer.configure() must be called before generate_player()")

        meta = self._meta
        rows, cols = meta.rows, meta.cols
        spacing = meta.cell_size_m

        terrain = self._get_terrain(rows, cols)
        grid_w = cols * spacing
        grid_h = rows * spacing
        max_height = float(terrain.max()) if terrain.size else 0.0
        viewpoint, orientation = self._compute_viewpoint(grid_w, grid_h, max_height)
        height_range = float(terrain.max() - terrain.min()) if terrain.size else 0.0
        ts = self._terrain_style(height_range)
        ws = self._water_style()
        terrain_str = self._heights_to_str(terrain)

        # Embed all water frames as a JS array of strings
        frames_js = ",\n  ".join(f'"{f}"' for f in water_frames)
        n_frames = len(water_frames)

        scene = self._build_scene(cols, rows, spacing, viewpoint, orientation, ts, ws, terrain_str,
                                  water_frames[0] if water_frames else terrain_str,
                                  water_def="WaterGrid")

        return f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8"/>
  <title>DanaSim Player — {rows}x{cols}</title>
  <script src="https://cdn.jsdelivr.net/npm/x_ite@12.1.4/dist/x_ite.min.js"></script>
  <style>
    * {{ box-sizing: border-box; margin: 0; padding: 0; }}
    body {{ background: #1a1a2e; color: #e0e0e0; font-family: sans-serif;
           display: flex; flex-direction: column; height: 100vh; }}
    #viewport {{ flex: 1; min-height: 0; overflow: hidden; }}
    x3d-canvas {{ width: 100%; height: 100%; display: block; }}
    #controls {{
      padding: 10px 16px; background: #16213e;
      display: flex; align-items: center; gap: 14px; flex-wrap: wrap;
      border-top: 1px solid #0f3460;
    }}
    #controls button {{
      background: #0f3460; color: #e0e0e0; border: 1px solid #1a6ea8;
      border-radius: 4px; padding: 5px 14px; cursor: pointer; font-size: 14px;
    }}
    #controls button:hover {{ background: #1a6ea8; }}
    #slider {{ flex: 1; min-width: 120px; accent-color: #1a6ea8; }}
    #speed  {{ width: 80px; accent-color: #1a6ea8; }}
    label {{ font-size: 12px; color: #a0a0c0; }}
    #frame-label {{ font-size: 13px; min-width: 80px; }}
  </style>
</head>
<body>
  <!-- rows={rows} cols={cols} cell_size={spacing:.4f}m  frames={n_frames} -->
  <div id="viewport">
    <x3d-canvas>
{scene}
    </x3d-canvas>
  </div>
  <div id="controls">
    <button id="prevBtn">&#9664;</button>
    <button id="playBtn">&#9654; Play</button>
    <button id="nextBtn">&#9654;&#9654;</button>
    <input id="slider" type="range" min="0" max="{n_frames - 1}" value="0"/>
    <span id="frame-label">Frame 0 / {n_frames - 1}</span>
    <label>Speed <input id="speed" type="range" min="100" max="2000" value="600" step="100"/></label>
  </div>

  <script>
    const waterFrames = [
  {frames_js}
    ];

    let current = 0;
    let playing = false;
    let timer = null;

    function setFrame(i) {{
      current = ((i % waterFrames.length) + waterFrames.length) % waterFrames.length;
      const grid = document.querySelector("ElevationGrid[DEF='WaterGrid']");
      if (grid) grid.setAttribute("height", waterFrames[current]);
      document.getElementById("slider").value = current;
      document.getElementById("frame-label").textContent =
        "Frame " + current + " / " + (waterFrames.length - 1);
    }}

    function startPlay() {{
      const ms = parseInt(document.getElementById("speed").value);
      timer = setInterval(() => setFrame(current + 1), ms);
      playing = true;
      document.getElementById("playBtn").textContent = "⏸ Pause";
    }}

    function stopPlay() {{
      clearInterval(timer);
      playing = false;
      document.getElementById("playBtn").textContent = "▶ Play";
    }}

    document.getElementById("playBtn").addEventListener("click", () => {{
      playing ? stopPlay() : startPlay();
    }});
    document.getElementById("prevBtn").addEventListener("click", () => {{
      stopPlay(); setFrame(current - 1);
    }});
    document.getElementById("nextBtn").addEventListener("click", () => {{
      stopPlay(); setFrame(current + 1);
    }});
    document.getElementById("slider").addEventListener("input", e => {{
      stopPlay(); setFrame(parseInt(e.target.value));
    }});
    document.getElementById("speed").addEventListener("change", () => {{
      if (playing) {{ stopPlay(); startPlay(); }}
    }});

    // Set initial frame once scene is ready
    document.querySelector("x3d-canvas").addEventListener("load", () => setFrame(0));
  </script>
</body>
</html>
"""

    # ------------------------------------------------------------------
    # Private helpers
    # ------------------------------------------------------------------

    def _terrain_style(self, height_range: float) -> _TerrainStyle:
        r, g, b = self._colors.terrain_rgb(height_range)
        return _TerrainStyle(f"{r:.2f} {g:.2f} {b:.2f}")

    def _water_style(self) -> _TerrainStyle:
        r, g, b = self._colors.water_rgb
        return _TerrainStyle(
            f"{r:.2f} {g:.2f} {b:.2f}",
            shininess=self._colors.water_shininess,
            transparency=self._colors.water_transparency,
        )

    def _build_scene(self, cols: int, rows: int, spacing: float,
                     viewpoint: str, orientation: str,
                     ts: _TerrainStyle, ws: _TerrainStyle,
                     terrain_str: str, water_str: str,
                     water_def: str = "") -> str:
        def_attr = f' DEF="{water_def}"' if water_def else ""
        r, g, b = self._colors.sky
        sky_str = f"{r:.2f} {g:.2f} {b:.2f}"
        return f"""<x3d>
  <scene>
    <background skyColor="{sky_str}"></background>
    <Viewpoint position="{viewpoint}" orientation="{orientation}" description="Overview"></Viewpoint>
    <NavigationInfo type='"EXAMINE" "ANY"'></NavigationInfo>
    <Shape>
      <Appearance>
        <Material diffuseColor="{ts.diffuse}" shininess="{ts.shininess:.2f}" transparency="{ts.transparency:.2f}"></Material>
      </Appearance>
      <ElevationGrid xDimension="{cols}" zDimension="{rows}"
        xSpacing="{spacing:.4f}" zSpacing="{spacing:.4f}"
        height="{terrain_str}" solid="false">
      </ElevationGrid>
    </Shape>
    <Shape>
      <Appearance>
        <Material diffuseColor="{ws.diffuse}" shininess="{ws.shininess:.2f}" transparency="{ws.transparency:.2f}"></Material>
      </Appearance>
      <ElevationGrid{def_attr} xDimension="{cols}" zDimension="{rows}"
        xSpacing="{spacing:.4f}" zSpacing="{spacing:.4f}"
        height="{water_str}" solid="false">
      </ElevationGrid>
    </Shape>
  </scene>
</x3d>"""

    def _get_terrain(self, rows: int, cols: int) -> np.ndarray:
        meta = self._meta
        if meta.terrain_heights is not None and meta.terrain_heights.size == rows * cols:
            raw = meta.terrain_heights.reshape(-1).astype(np.float32)
        else:
            raw = np.zeros(rows * cols, dtype=np.float32)
        return self._sanitize(raw) * self._scale_z(rows, cols)

    def _get_water(self, water_depths: np.ndarray, rows: int, cols: int) -> np.ndarray:
        terrain = self._get_terrain(rows, cols)
        scale = self._scale_z(rows, cols)
        flat = self._sanitize(water_depths.flatten().astype(np.float32))
        flat = np.where(flat < _WATER_MIN_THRESHOLD, 0.0, flat) * scale
        # Dry cells: sink water 1 unit below terrain so it is occluded by terrain mesh
        below = terrain - 1.0
        return np.where(flat == 0.0, below, terrain + flat)

    def _scale_z(self, rows: int, cols: int) -> float:
        """Vertical exaggeration so terrain height ≈ 10% of the shorter grid side."""
        meta = self._meta
        if meta.terrain_heights is None:
            return 1.0
        raw = self._sanitize(meta.terrain_heights.reshape(-1).astype(np.float32))
        terrain_range = float(raw.max() - raw.min())
        if terrain_range < 0.01:
            return 1.0
        min_dim = min(rows * meta.cell_size_m, cols * meta.cell_size_m)
        return (min_dim * 0.10) / terrain_range

    def _sanitize(self, values: np.ndarray) -> np.ndarray:
        out = values.copy()
        out = np.where(np.isnan(out), 0.0, out)
        out = np.where(np.isposinf(out), 1000.0, out)
        out = np.where(np.isneginf(out), -1000.0, out)
        return np.clip(out, -1000.0, 10000.0)

    def _heights_to_str(self, values: np.ndarray) -> str:
        return " ".join(f"{v:.4f}" for v in values)

    def _compute_viewpoint(self, grid_w: float, grid_h: float, max_height: float = 0.0) -> str:
        cx = grid_w * 0.5
        cz = grid_h * 0.5
        max_dim = max(grid_w, grid_h)
        cam_x = cx                        # centred over the grid
        cam_y = max(max_height * 2.5, max_dim * 0.4)
        cam_z = cz + max_dim * 0.8        # behind the grid
        angle = -math.atan2(cam_y, max_dim * 0.8)
        return f"{cam_x:.2f} {cam_y:.2f} {cam_z:.2f}", f"1 0 0 {angle:.3f}"

    def _wrap_html(self, scene: str, meta: GridMeta) -> str:
        return f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8"/>
  <title>DanaSim — {meta.rows}x{meta.cols} grid</title>
  <script src="https://cdn.jsdelivr.net/npm/x_ite@12.1.4/dist/x_ite.min.js"></script>
  <style>
    html, body {{ margin: 0; height: 100%; background: #1a1a2e; }}
    x3d-canvas {{ width: 100vw; height: 100vh; display: block; }}
  </style>
</head>
<body>
  <!-- rows={meta.rows} cols={meta.cols} cell_size={meta.cell_size_m:.4f}m -->
  <x3d-canvas>
{scene}
  </x3d-canvas>
</body>
</html>
"""
