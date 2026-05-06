"""Generate X3D heightmap viewer from DanaSim CSV data (Mario-style approach).

Instead of writing terrain heights as ASCII strings in the HTML, terrain is
encoded as a grayscale PNG and the browser decodes it in JavaScript to build
LOD ElevationGrid geometry — the same idea as Mario's version 5 but with our
chunk/LOD system and flood animation added on top.

Size comparison vs ASCII LOD output:
  ASCII (subsample 4/16):  ~148 MB per frame (text in frame tokens)
  This tool:               ~2 MB terrain PNG (once) + ~0.1 MB per frame flood PNG

Run as:
    python -m python.visualizer.tools.generate_x3d_heightmap \\
        --csv outputs/csv_data/ \\
        --output outputs/x3d_heightmap/

Then serve and open:
    python -m http.server 8080
    # http://localhost:8080/outputs/x3d_heightmap/player.html
"""
from __future__ import annotations

import argparse
import base64
import json
import logging
import sys
from io import BytesIO
from pathlib import Path

import numpy as np
from PIL import Image

from .csv_utils import discover_frames, load_frame, load_meta, load_terrain

_NODATA      = -9999.0
_DEFAULT_RES = 5.0     # metres per PNG pixel — full source resolution for best zoom quality
_CHUNK_M     = 2500.0  # chunk size in metres


def _encode_terrain_png(terrain_2d: np.ndarray) -> tuple[bytes, float, float]:
    """Encode (rows, cols) float32 heights as 8-bit grayscale PNG."""
    valid = terrain_2d[terrain_2d > _NODATA]
    min_h = float(valid.min()) if valid.size else 0.0
    max_h = float(valid.max()) if valid.size else 100.0
    if max_h == min_h:
        max_h = min_h + 1.0
    norm = np.clip((terrain_2d - min_h) / (max_h - min_h), 0.0, 1.0)
    img = Image.fromarray((norm * 255).astype(np.uint8), mode="L")
    buf = BytesIO()
    img.save(buf, format="PNG", optimize=True)
    return buf.getvalue(), min_h, max_h


def _encode_flood_png(palette_2d: np.ndarray) -> bytes:
    """Encode (rows, cols) palette indices (0–5) as 8-bit grayscale PNG."""
    img = Image.fromarray(palette_2d.astype(np.uint8), mode="L")
    buf = BytesIO()
    img.save(buf, format="PNG", optimize=True)
    return buf.getvalue()


def _auto_detect_palette(csv_dir: Path) -> Path | None:
    for parent in csv_dir.parents:
        c = parent / "data" / "data_29_10_2024" / "color_palette.json"
        if c.exists():
            return c
    return None


def _load_state_colors(palette_path: Path | None) -> list[tuple[int, int, int]]:
    defaults = [
        (245, 222, 179),  # 0 dry
        (153, 204, 255),  # 1 very shallow
        (100, 150, 220),  # 2 low
        (30,  100, 200),  # 3 medium
        (0,    50, 180),  # 4 high
        (75,    0, 130),  # 5 extreme
    ]
    if palette_path is None or not palette_path.exists():
        return defaults
    try:
        data = json.loads(palette_path.read_text(encoding="utf-8"))
        fr = data.get("layers", {}).get("flood_risk", [])
        if fr:
            return [(e["rgba"][0], e["rgba"][1], e["rgba"][2])
                    for e in sorted(fr, key=lambda e: e["value"])]
        x3d = data.get("x3d", {}).get("state_colors", [])
        if x3d:
            return [(int(r * 255), int(g * 255), int(b * 255)) for r, g, b in x3d]
    except Exception:
        pass
    return defaults


# Approximate water surface lift (m) per palette state — gives visual depth cues.
_WATER_LIFT = [0.0, 0.1, 0.4, 1.0, 2.5, 5.0]


def _generate_player(
    png_cols: int, png_rows: int, png_res_m: float,
    orig_cols: int, orig_rows: int, cell_size_m: float,
    min_h: float, max_h: float,
    terrain_b64: str,
    frame_names: list[str],
    state_colors: list[tuple[int, int, int]],
) -> str:
    map_w = orig_cols * cell_size_m
    map_d = orig_rows * cell_size_m
    cam_h = max(max_h * 3.0, map_d * 0.3)

    # LOD mesh cell sizes — independent of PNG resolution.
    # PNG_RES controls sampling accuracy; these control ElevationGrid vertex density.
    # Matching Mario's v5: 25m / 100m / 500m.
    lod_cell_sizes = [25.0, 100.0, 500.0]
    lod_ranges     = "3000,10000,30000"

    sc_js    = "[\n" + ",\n".join(
        f'  "{r/255:.2f} {g/255:.2f} {b/255:.2f}"' for r, g, b in state_colors
    ) + "\n]"
    n_frames = len(frame_names)

    return f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8"/>
  <title>DanaSim Heightmap Viewer</title>
  <script src="https://cdn.jsdelivr.net/npm/x_ite@12.1.4/dist/x_ite.min.js"></script>
  <style>
    * {{ box-sizing: border-box; margin: 0; padding: 0; }}
    body {{ background: #1a1a2e; color: #e0e0e0; font-family: sans-serif;
           display: flex; flex-direction: column; height: 100vh; }}
    #viewport {{ flex: 1; min-height: 0; overflow: hidden; position: relative; }}
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
    #frame-label {{ font-size: 13px; min-width: 100px; }}
    #status {{ font-size: 11px; color: #6080a0; min-width: 160px; }}
    #loading {{
      position: absolute; top: 50%; left: 50%; transform: translate(-50%,-50%);
      color: white; font-size: 18px; background: rgba(0,0,0,0.85);
      padding: 24px 32px; border-radius: 8px; z-index: 10; text-align: center;
    }}
  </style>
</head>
<body>
  <div id="loading">Decoding heightmap and building 3D terrain…</div>
  <div id="viewport">
    <x3d-canvas>
      <x3d>
        <scene>
          <background skyColor="0.53 0.81 0.98"></background>
          <Viewpoint position="{map_w/2:.0f} {cam_h:.0f} {map_d + map_d * 0.4:.0f}"
                     orientation="1 0 0 -0.5" description="Overview"></Viewpoint>
          <NavigationInfo type='"EXAMINE" "ANY"' speed="500"></NavigationInfo>
          <Group id="terrain_container"></Group>
        </scene>
      </x3d>
    </x3d-canvas>
  </div>
  <div id="controls">
    <button id="prevBtn">&#9664;</button>
    <button id="playBtn">&#9654; Play</button>
    <button id="nextBtn">&#9654;&#9654;</button>
    <input id="slider" type="range" min="0" max="{n_frames - 1}" value="0"/>
    <span id="frame-label">Frame 0 / {n_frames - 1}</span>
    <label>Speed <input id="speed" type="range" min="100" max="2000" value="800" step="100"/></label>
    <span id="status">Loading terrain…</span>
  </div>

<script>
const PNG_COLS   = {png_cols};
const PNG_ROWS   = {png_rows};
const PNG_RES    = {png_res_m};
const MAP_W      = {map_w};
const MAP_D      = {map_d};
const MIN_H      = {min_h:.4f};
const MAX_H      = {max_h:.4f};
const CHUNK_M    = {_CHUNK_M};
const LOD_RANGES = "{lod_ranges}";
const LOD_SIZES  = {json.dumps(lod_cell_sizes)};
const STATE_COLORS = {sc_js};
const WATER_LIFT = {json.dumps(_WATER_LIFT)};
const FLOOD_FRAMES = {json.dumps(frame_names)};

// ---------------------------------------------------------------------------
// Decode embedded terrain PNG, then fetch frame 0 flood PNG before building
// ---------------------------------------------------------------------------
let _terrainPx = null;
let _floodCanvas = null, _floodCtx = null;

async function _decodeImage(src) {{
  const img = new Image();
  await new Promise(r => {{ img.onload = r; img.src = src; }});
  const c = document.createElement("canvas");
  c.width = img.width; c.height = img.height;
  c.getContext("2d").drawImage(img, 0, 0);
  return c.getContext("2d").getImageData(0, 0, img.width, img.height).data;
}}

async function _fetchFloodPx(frameName) {{
  try {{
    const resp = await fetch(`flood/${{frameName}}.png`);
    if (!resp.ok) return null;
    const bitmap = await createImageBitmap(await resp.blob());
    if (!_floodCanvas) {{
      _floodCanvas = document.createElement("canvas");
      _floodCanvas.width = PNG_COLS; _floodCanvas.height = PNG_ROWS;
      _floodCtx = _floodCanvas.getContext("2d");
    }}
    _floodCtx.drawImage(bitmap, 0, 0);
    return _floodCtx.getImageData(0, 0, PNG_COLS, PNG_ROWS).data;
  }} catch(e) {{ return null; }}
}}

(async () => {{
  try {{
    document.getElementById("loading").textContent = "Step 1/3: Decoding terrain PNG…";
    _terrainPx = await _decodeImage("data:image/png;base64,{terrain_b64}");

    document.getElementById("loading").textContent = "Step 2/3: Fetching flood data…";
    const initFlood = FLOOD_FRAMES.length > 0
      ? await _fetchFloodPx(FLOOD_FRAMES[0])
      : null;

    document.getElementById("loading").textContent = "Step 3/3: Building 3D scene…";
    await buildScene(initFlood);
  }} catch(err) {{
    document.getElementById("loading").textContent = "Error: " + err.message;
    console.error("Scene init failed:", err);
  }}
}})();

function getTerrainH(px, pz) {{
  if (px >= PNG_COLS) px = PNG_COLS - 1;
  if (pz >= PNG_ROWS) pz = PNG_ROWS - 1;
  return (_terrainPx[(pz * PNG_COLS + px) * 4] / 255.0) * (MAX_H - MIN_H) + MIN_H;
}}

function terrainColor(h) {{
  if (h <= 0)   return "0.20 0.39 0.70";
  if (h <= 5)   return "0.82 0.76 0.57";
  if (h <= 30)  return "0.73 0.80 0.51";
  if (h <= 100) return "0.55 0.67 0.36";
  return "0.51 0.44 0.29";
}}

// ---------------------------------------------------------------------------
// Node cache — populated after innerHTML set, avoids repeated querySelector
// ---------------------------------------------------------------------------
const _gridNodes  = new Map();  // DEF key -> ElevationGrid element
const _colorNodes = new Map();  // DEF+"_C" key -> Color element

function _cacheNodes() {{
  document.querySelectorAll("ElevationGrid[DEF]").forEach(el =>
    _gridNodes.set(el.getAttribute("DEF"), el));
  document.querySelectorAll("Color[DEF]").forEach(el =>
    _colorNodes.set(el.getAttribute("DEF"), el));
}}

// ---------------------------------------------------------------------------
// Chunk building — all LOD levels built synchronously (Mario's approach)
// ---------------------------------------------------------------------------
function _buildLodShape(cxi, czi, cellM, terrainOnly, floodPx) {{
  const tx = cxi * CHUNK_M, tz = czi * CHUNK_M;
  const aw = Math.min(CHUNK_M, MAP_W - tx);
  const ad = Math.min(CHUNK_M, MAP_D - tz);
  const pts_x = Math.floor(aw / cellM) + 1;
  const pts_z = Math.floor(ad / cellM) + 1;
  const heights = [], colors = [];

  for (let lz = 0; lz < pts_z; lz++) {{
    for (let lx = 0; lx < pts_x; lx++) {{
      const px = Math.min(Math.floor((tx + lx * cellM) / PNG_RES), PNG_COLS - 1);
      const pz = Math.min(Math.floor((tz + lz * cellM) / PNG_RES), PNG_ROWS - 1);
      const h  = getTerrainH(px, pz);
      const st = (!terrainOnly && floodPx) ? floodPx[(pz * PNG_COLS + px) * 4] : 0;
      heights.push((h + (st > 0 ? WATER_LIFT[Math.min(st, WATER_LIFT.length-1)] : 0)).toFixed(2));
      colors.push(st > 0 ? STATE_COLORS[Math.min(st, STATE_COLORS.length-1)] : terrainColor(h));
    }}
  }}

  const def = `C${{cxi}}_${{czi}}_${{cellM}}`;
  return {{ def, pts_x, pts_z, cellM, heights, colors }};
}}

function _shapeHTML({{def, pts_x, pts_z, cellM, heights, colors}}) {{
  return `<Shape>
    <Appearance><Material ambientIntensity="1" diffuseColor="1 1 1"/></Appearance>
    <ElevationGrid DEF="${{def}}"
      xDimension="${{pts_x}}" zDimension="${{pts_z}}"
      xSpacing="${{cellM}}" zSpacing="${{cellM}}"
      height="${{heights.join(" ")}}" solid="false">
      <Color DEF="${{def}}_C" color="${{colors.join(" ")}}"/>
    </ElevationGrid>
  </Shape>`;
}}

function buildChunk(cxi, czi, floodPx) {{
  const tx = cxi * CHUNK_M, tz = czi * CHUNK_M;
  const aw = Math.min(CHUNK_M, MAP_W - tx);
  const ad = Math.min(CHUNK_M, MAP_D - tz);
  let lodShapes = "";

  for (const cellM of LOD_SIZES) {{
    lodShapes += _shapeHTML(_buildLodShape(cxi, czi, cellM, false, floodPx));
  }}
  lodShapes += '<WorldInfo info="fuera_de_rango"/>';

  return `<Transform translation="${{tx}} 0 ${{tz}}">
    <LOD range="${{LOD_RANGES}}" center="${{(aw/2).toFixed(1)}} 0 ${{(ad/2).toFixed(1)}}">
      ${{lodShapes}}
    </LOD>
  </Transform>`;
}}

async function buildScene(initFloodPx) {{
  const cxSteps = Math.ceil(MAP_W / CHUNK_M);
  const czSteps = Math.ceil(MAP_D / CHUNK_M);
  const total = cxSteps * czSteps;
  const parts = [];

  for (let cz = 0; cz < czSteps; cz++) {{
    for (let cx = 0; cx < cxSteps; cx++) {{
      parts.push(buildChunk(cx, cz, initFloodPx));
    }}
    // Yield once per row so the browser updates the loading text
    document.getElementById("loading").textContent =
      `Building scene… ${{(cz + 1) * cxSteps}}/${{total}} chunks`;
    await new Promise(r => setTimeout(r, 0));
  }}

  document.getElementById("terrain_container").innerHTML = parts.join("");
  _cacheNodes();
  document.getElementById("loading").style.display = "none";
  document.getElementById("status").textContent = `${{FLOOD_FRAMES.length}} frame(s) ready`;
  current = 0;
}}

// ---------------------------------------------------------------------------
// Flood overlay — async, cancellable, uses node cache
// ---------------------------------------------------------------------------
let _floodSeq = 0;

async function applyFlood(floodPx) {{
  const mySeq = ++_floodSeq;
  const cxSteps = Math.ceil(MAP_W / CHUNK_M);
  const czSteps = Math.ceil(MAP_D / CHUNK_M);

  for (let czi = 0; czi < czSteps; czi++) {{
    for (let cxi = 0; cxi < cxSteps; cxi++) {{
      if (mySeq !== _floodSeq) return;  // superseded by newer frame

      for (const cellM of LOD_SIZES) {{
        const def  = `C${{cxi}}_${{czi}}_${{cellM}}`;
        const grid = _gridNodes.get(def);
        const col  = _colorNodes.get(def + "_C");
        if (!grid || !col) continue;  // not yet built

        const data = _buildLodShape(cxi, czi, cellM, false, floodPx);
        grid.setAttribute("height", data.heights.join(" "));
        col.setAttribute("color",  data.colors.join(" "));
      }}
    }}
    await new Promise(r => setTimeout(r, 0));  // yield after each row of chunks
  }}
}}

// ---------------------------------------------------------------------------
// Player controls
// ---------------------------------------------------------------------------
let current = 0, playing = false, timer = null;

async function setFrame(i) {{
  const next = ((i % FLOOD_FRAMES.length) + FLOOD_FRAMES.length) % FLOOD_FRAMES.length;
  document.getElementById("slider").value = next;
  document.getElementById("frame-label").textContent =
    `Frame ${{next}} / ${{FLOOD_FRAMES.length - 1}}`;

  if (next === current) return;  // already showing this frame (baked at startup)
  current = next;

  const floodPx = await _fetchFloodPx(FLOOD_FRAMES[current]);
  applyFlood(floodPx);
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

document.getElementById("playBtn").addEventListener("click", () => playing ? stopPlay() : startPlay());
document.getElementById("prevBtn").addEventListener("click", () => {{ stopPlay(); setFrame(current - 1); }});
document.getElementById("nextBtn").addEventListener("click", () => {{ stopPlay(); setFrame(current + 1); }});
document.getElementById("slider").addEventListener("input", e => {{ stopPlay(); setFrame(parseInt(e.target.value)); }});
document.getElementById("speed").addEventListener("change", () => {{ if (playing) {{ stopPlay(); startPlay(); }} }});
</script>
</body>
</html>
"""


def main(argv: list[str] | None = None) -> int:
    logging.basicConfig(level=logging.INFO, format="%(levelname)s %(message)s")
    log = logging.getLogger(__name__)

    parser = argparse.ArgumentParser(
        description="Generate X3D heightmap viewer from DanaSim CSV data.",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("--csv", required=True, help="Path to csv_data/ directory")
    parser.add_argument("--output", default="outputs/x3d_heightmap",
                        help="Output directory")
    parser.add_argument("--resolution", type=float, default=_DEFAULT_RES,
                        help="PNG resolution in metres (downsampling factor for terrain and flood)")
    parser.add_argument("--frames", type=int, default=None,
                        help="Limit to first N frames")
    parser.add_argument("--palette", default=None,
                        help="Path to color_palette.json (auto-detected if omitted)")
    args = parser.parse_args(argv)

    csv_dir    = Path(args.csv).resolve()
    output_dir = Path(args.output).resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    meta      = load_meta(csv_dir)
    rows, cols = meta["rows"], meta["cols"]
    cell_size  = meta["cell_size_m"]
    factor     = max(1, round(args.resolution / cell_size))
    actual_res = cell_size * factor
    log.info("Grid: %dx%d  cell_size=%.2fm  PNG res=%.1fm (downsample ×%d)",
             rows, cols, cell_size, actual_res, factor)

    terrain    = load_terrain(csv_dir)
    terrain_2d = np.where(
        terrain.reshape(rows, cols) < -9000.0, 0.0,
        terrain.reshape(rows, cols)
    ).astype(np.float32)
    terrain_dn = terrain_2d[::factor, ::factor]
    d_rows, d_cols = terrain_dn.shape
    log.info("Terrain PNG: %dx%d pixels", d_rows, d_cols)

    terrain_png, min_h, max_h = _encode_terrain_png(terrain_dn)
    log.info("Heights: min=%.2fm  max=%.2fm  PNG=%.1fKB",
             min_h, max_h, len(terrain_png) / 1024)
    terrain_b64 = base64.b64encode(terrain_png).decode("ascii")

    palette_path = Path(args.palette) if args.palette else _auto_detect_palette(csv_dir)
    state_colors = _load_state_colors(palette_path)
    log.info("State colors: %d states", len(state_colors))

    frame_paths = discover_frames(csv_dir)
    if args.frames is not None:
        frame_paths = frame_paths[: args.frames]
    if not frame_paths:
        log.error("No step_XXXXX.csv files found in %s", csv_dir)
        return 1

    flood_dir = output_dir / "flood"
    flood_dir.mkdir(parents=True, exist_ok=True)
    frame_names: list[str] = []

    for fp in frame_paths:
        step_name = fp.stem
        palette_grid, _ = load_frame(fp, rows, cols)
        palette_dn  = palette_grid[::factor, ::factor]
        flood_png   = _encode_flood_png(palette_dn)
        (flood_dir / f"{step_name}.png").write_bytes(flood_png)
        frame_names.append(step_name)
        log.info("Frame %s — flood PNG %.1fKB", step_name, len(flood_png) / 1024)

    player_html = _generate_player(
        d_cols, d_rows, actual_res,
        cols, rows, cell_size,
        min_h, max_h,
        terrain_b64,
        frame_names,
        state_colors,
    )
    player_path = output_dir / "player.html"
    player_path.write_text(player_html, encoding="utf-8")
    log.info("Player: %s  (%.1fKB)", player_path, player_path.stat().st_size / 1024)
    log.info("Done — %d frames processed", len(frame_names))
    return 0


if __name__ == "__main__":
    sys.exit(main())
