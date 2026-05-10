// DanaSim X3D heightmap player
// Phase 1: identical behaviour to the previous monolithic generator.
// Configuration is injected by the Python generator into window.__CONFIG__.

const CFG = window.__CONFIG__;

const PNG_COLS     = CFG.pngCols;
const PNG_ROWS     = CFG.pngRows;
const PNG_RES      = CFG.pngRes;
const MAP_W        = CFG.mapW;
const MAP_D        = CFG.mapD;
const MIN_H        = CFG.minH;
const MAX_H        = CFG.maxH;
const CHUNK_M      = CFG.chunkM;
const LOD_RANGES   = CFG.lodRanges;
const LOD_SIZES    = CFG.lodSizes;
const STATE_COLORS = CFG.stateColors;
const WATER_LIFT   = CFG.waterLift;
const FLOOD_FRAMES = CFG.floodFrames;
const TERRAIN_SRC  = CFG.terrainSrc;

// ---------------------------------------------------------------------------
// Decode embedded terrain PNG, then fetch frame 0 flood PNG before building
// ---------------------------------------------------------------------------
let _terrainPx = null;
let _floodCanvas = null, _floodCtx = null;

async function _decodeImage(src) {
  const img = new Image();
  await new Promise(r => { img.onload = r; img.src = src; });
  const c = document.createElement("canvas");
  c.width = img.width; c.height = img.height;
  c.getContext("2d").drawImage(img, 0, 0);
  return c.getContext("2d").getImageData(0, 0, img.width, img.height).data;
}

async function _fetchFloodPx(frameName) {
  try {
    const resp = await fetch(`flood/${frameName}.png`);
    if (!resp.ok) return null;
    const bitmap = await createImageBitmap(await resp.blob());
    if (!_floodCanvas) {
      _floodCanvas = document.createElement("canvas");
      _floodCanvas.width = PNG_COLS; _floodCanvas.height = PNG_ROWS;
      _floodCtx = _floodCanvas.getContext("2d");
    }
    _floodCtx.drawImage(bitmap, 0, 0);
    return _floodCtx.getImageData(0, 0, PNG_COLS, PNG_ROWS).data;
  } catch(e) { return null; }
}

(async () => {
  try {
    document.getElementById("loading").textContent = "Step 1/3: Decoding terrain PNG…";
    _terrainPx = await _decodeImage(TERRAIN_SRC);

    document.getElementById("loading").textContent = "Step 2/3: Fetching flood data…";
    const initFlood = FLOOD_FRAMES.length > 0
      ? await _fetchFloodPx(FLOOD_FRAMES[0])
      : null;

    document.getElementById("loading").textContent = "Step 3/3: Building 3D scene…";
    await buildScene(initFlood);
  } catch(err) {
    document.getElementById("loading").textContent = "Error: " + err.message;
    console.error("Scene init failed:", err);
  }
})();

function getTerrainH(px, pz) {
  if (px >= PNG_COLS) px = PNG_COLS - 1;
  if (pz >= PNG_ROWS) pz = PNG_ROWS - 1;
  return (_terrainPx[(pz * PNG_COLS + px) * 4] / 255.0) * (MAX_H - MIN_H) + MIN_H;
}

function terrainColor(h) {
  if (h <= 0)   return "0.20 0.39 0.70";
  if (h <= 5)   return "0.82 0.76 0.57";
  if (h <= 30)  return "0.73 0.80 0.51";
  if (h <= 100) return "0.55 0.67 0.36";
  return "0.51 0.44 0.29";
}

// ---------------------------------------------------------------------------
// Node cache — populated after innerHTML set, avoids repeated querySelector
// ---------------------------------------------------------------------------
const _gridNodes  = new Map();  // DEF key -> ElevationGrid element
const _colorNodes = new Map();  // DEF+"_C" key -> Color element

function _cacheNodes() {
  document.querySelectorAll("ElevationGrid[DEF]").forEach(el =>
    _gridNodes.set(el.getAttribute("DEF"), el));
  document.querySelectorAll("Color[DEF]").forEach(el =>
    _colorNodes.set(el.getAttribute("DEF"), el));
}

// ---------------------------------------------------------------------------
// Chunk building — all LOD levels built synchronously (Mario's approach)
// ---------------------------------------------------------------------------
function _buildLodShape(cxi, czi, cellM, terrainOnly, floodPx) {
  const tx = cxi * CHUNK_M, tz = czi * CHUNK_M;
  const aw = Math.min(CHUNK_M, MAP_W - tx);
  const ad = Math.min(CHUNK_M, MAP_D - tz);
  const pts_x = Math.floor(aw / cellM) + 1;
  const pts_z = Math.floor(ad / cellM) + 1;
  const heights = [], colors = [];

  for (let lz = 0; lz < pts_z; lz++) {
    for (let lx = 0; lx < pts_x; lx++) {
      const px = Math.min(Math.floor((tx + lx * cellM) / PNG_RES), PNG_COLS - 1);
      const pz = Math.min(Math.floor((tz + lz * cellM) / PNG_RES), PNG_ROWS - 1);
      const h  = getTerrainH(px, pz);
      const st = (!terrainOnly && floodPx) ? floodPx[(pz * PNG_COLS + px) * 4] : 0;
      heights.push((h + (st > 0 ? WATER_LIFT[Math.min(st, WATER_LIFT.length-1)] : 0)).toFixed(2));
      colors.push(st > 0 ? STATE_COLORS[Math.min(st, STATE_COLORS.length-1)] : terrainColor(h));
    }
  }

  const def = `C${cxi}_${czi}_${cellM}`;
  return { def, pts_x, pts_z, cellM, heights, colors };
}

function _shapeHTML({def, pts_x, pts_z, cellM, heights, colors}) {
  return `<Shape>
    <Appearance><Material ambientIntensity="1" diffuseColor="1 1 1"/></Appearance>
    <ElevationGrid DEF="${def}"
      xDimension="${pts_x}" zDimension="${pts_z}"
      xSpacing="${cellM}" zSpacing="${cellM}"
      height="${heights.join(" ")}" solid="false">
      <Color DEF="${def}_C" color="${colors.join(" ")}"/>
    </ElevationGrid>
  </Shape>`;
}

function buildChunk(cxi, czi, floodPx) {
  const tx = cxi * CHUNK_M, tz = czi * CHUNK_M;
  const aw = Math.min(CHUNK_M, MAP_W - tx);
  const ad = Math.min(CHUNK_M, MAP_D - tz);
  let lodShapes = "";

  for (const cellM of LOD_SIZES) {
    lodShapes += _shapeHTML(_buildLodShape(cxi, czi, cellM, false, floodPx));
  }
  lodShapes += '<WorldInfo info="fuera_de_rango"/>';

  return `<Transform translation="${tx} 0 ${tz}">
    <LOD range="${LOD_RANGES}" center="${(aw/2).toFixed(1)} 0 ${(ad/2).toFixed(1)}">
      ${lodShapes}
    </LOD>
  </Transform>`;
}

async function buildScene(initFloodPx) {
  const cxSteps = Math.ceil(MAP_W / CHUNK_M);
  const czSteps = Math.ceil(MAP_D / CHUNK_M);
  const total = cxSteps * czSteps;
  const parts = [];

  for (let cz = 0; cz < czSteps; cz++) {
    for (let cx = 0; cx < cxSteps; cx++) {
      parts.push(buildChunk(cx, cz, initFloodPx));
    }
    // Yield once per row so the browser updates the loading text
    document.getElementById("loading").textContent =
      `Building scene… ${(cz + 1) * cxSteps}/${total} chunks`;
    await new Promise(r => setTimeout(r, 0));
  }

  document.getElementById("terrain_container").innerHTML = parts.join("");
  _cacheNodes();
  document.getElementById("loading").style.display = "none";
  document.getElementById("status").textContent = `${FLOOD_FRAMES.length} frame(s) ready`;
  current = 0;
}

// ---------------------------------------------------------------------------
// Flood overlay — async, cancellable, uses node cache
// ---------------------------------------------------------------------------
let _floodSeq = 0;

async function applyFlood(floodPx) {
  const mySeq = ++_floodSeq;
  const cxSteps = Math.ceil(MAP_W / CHUNK_M);
  const czSteps = Math.ceil(MAP_D / CHUNK_M);

  for (let czi = 0; czi < czSteps; czi++) {
    for (let cxi = 0; cxi < cxSteps; cxi++) {
      if (mySeq !== _floodSeq) return;  // superseded by newer frame

      for (const cellM of LOD_SIZES) {
        const def  = `C${cxi}_${czi}_${cellM}`;
        const grid = _gridNodes.get(def);
        const col  = _colorNodes.get(def + "_C");
        if (!grid || !col) continue;  // not yet built

        const data = _buildLodShape(cxi, czi, cellM, false, floodPx);
        grid.setAttribute("height", data.heights.join(" "));
        col.setAttribute("color",  data.colors.join(" "));
      }
    }
    await new Promise(r => setTimeout(r, 0));  // yield after each row of chunks
  }
}

// ---------------------------------------------------------------------------
// Player controls
// ---------------------------------------------------------------------------
let current = 0, playing = false, timer = null;

async function setFrame(i) {
  const next = ((i % FLOOD_FRAMES.length) + FLOOD_FRAMES.length) % FLOOD_FRAMES.length;
  document.getElementById("slider").value = next;
  document.getElementById("frame-label").textContent =
    `Frame ${next} / ${FLOOD_FRAMES.length - 1}`;

  if (next === current) return;  // already showing this frame (baked at startup)
  current = next;

  const floodPx = await _fetchFloodPx(FLOOD_FRAMES[current]);
  applyFlood(floodPx);
}

function startPlay() {
  const ms = parseInt(document.getElementById("speed").value);
  timer = setInterval(() => setFrame(current + 1), ms);
  playing = true;
  document.getElementById("playBtn").textContent = "⏸ Pause";
}
function stopPlay() {
  clearInterval(timer);
  playing = false;
  document.getElementById("playBtn").textContent = "▶ Play";
}

document.getElementById("playBtn").addEventListener("click", () => playing ? stopPlay() : startPlay());
document.getElementById("prevBtn").addEventListener("click", () => { stopPlay(); setFrame(current - 1); });
document.getElementById("nextBtn").addEventListener("click", () => { stopPlay(); setFrame(current + 1); });
document.getElementById("slider").addEventListener("input", e => { stopPlay(); setFrame(parseInt(e.target.value)); });
document.getElementById("speed").addEventListener("change", () => { if (playing) { stopPlay(); startPlay(); } });
