// Scene assembly — drives the initial bake and incremental flood updates.
// Holds DOM-side state (node cache, async sequence number) so the rest of
// the codebase stays free of side effects.

import { CHUNK_M, LOD_SIZES } from "../config.js";
import { MAP_W, MAP_D } from "../runtime.js";
import { buildChunk, buildLodShape } from "./builder.js";

/** Cache of `<ElevationGrid DEF=…>` nodes keyed by chunk DEF. */
const _gridNodes = new Map();
/** Cache of `<Color DEF=…>` nodes keyed by chunk DEF (with `_C` suffix). */
const _colorNodes = new Map();

/** Sequence number used to cancel in-flight `applyFlood` calls when a newer
 *  frame arrives. Each call captures its `mySeq` and bails when superseded. */
let _floodSeq = 0;

function _cacheNodes() {
  document.querySelectorAll("ElevationGrid[DEF]").forEach(el =>
    _gridNodes.set(el.getAttribute("DEF"), el));
  document.querySelectorAll("Color[DEF]").forEach(el =>
    _colorNodes.set(el.getAttribute("DEF"), el));
}

/**
 * Build every chunk and inject the resulting markup into the page.
 * Yields once per chunk row so the loading indicator can update.
 *
 * @param {Uint8ClampedArray | null} initFloodPx initial frame (frame 0)
 * @param {(message: string) => void} [onProgress] optional progress callback
 */
export async function buildScene(initFloodPx, onProgress) {
  const cxSteps = Math.ceil(MAP_W / CHUNK_M);
  const czSteps = Math.ceil(MAP_D / CHUNK_M);
  const total = cxSteps * czSteps;
  const parts = [];

  for (let cz = 0; cz < czSteps; cz++) {
    for (let cx = 0; cx < cxSteps; cx++) {
      parts.push(buildChunk(cx, cz, initFloodPx));
    }
    if (onProgress) {
      onProgress(`Building scene… ${(cz + 1) * cxSteps}/${total} chunks`);
    }
    await new Promise(r => setTimeout(r, 0));
  }

  document.getElementById("terrain_container").innerHTML = parts.join("");
  _cacheNodes();
}

/**
 * Apply a new flood frame on top of the already-built scene by mutating the
 * cached ElevationGrid/Color nodes in place. Cancellable: a later call with
 * a fresher frame supersedes any in-flight call.
 *
 * @param {Uint8ClampedArray | null} floodPx
 */
export async function applyFlood(floodPx) {
  const mySeq = ++_floodSeq;
  const cxSteps = Math.ceil(MAP_W / CHUNK_M);
  const czSteps = Math.ceil(MAP_D / CHUNK_M);

  for (let czi = 0; czi < czSteps; czi++) {
    for (let cxi = 0; cxi < cxSteps; cxi++) {
      if (mySeq !== _floodSeq) return;  // superseded by a newer frame

      for (const cellM of LOD_SIZES) {
        const def = `C${cxi}_${czi}_${cellM}`;
        const grid = _gridNodes.get(def);
        const col = _colorNodes.get(def + "_C");
        if (!grid || !col) continue;  // chunk not yet built

        const data = buildLodShape(cxi, czi, cellM, false, floodPx);
        grid.setAttribute("height", data.heights.join(" "));
        col.setAttribute("color", data.colors.join(" "));
      }
    }
    await new Promise(r => setTimeout(r, 0));  // yield after each chunk row
  }
}
