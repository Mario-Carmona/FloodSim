// Scene assembly — drives the initial bake and incremental updates.
//
// Holds DOM-side state (node cache, async sequence number, the latest flood
// pixel buffer) so consumers only think in terms of "the user changed the
// frame" or "the user toggled a layer" without juggling the actual data.

import { CHUNK_M, LOD_SIZES } from "../config.js";
import { MAP_W, MAP_D } from "../runtime.js";
import { buildChunk, buildLodShape } from "./builder.js";

/** @typedef {import("./builder.js").BuildOpts} BuildOpts */

/** Cache of `<ElevationGrid DEF=…>` nodes keyed by chunk DEF. */
const _gridNodes = new Map();
/** Cache of `<Color DEF=…>` nodes keyed by chunk DEF (with `_C` suffix). */
const _colorNodes = new Map();

/** Latest flood pixel buffer applied to the scene. Used by `setLayers` so the
 *  caller does not have to re-fetch the PNG when only toggles changed. */
let _floodPx = null;

/** Sequence counter used to cancel stale `applyOpts` calls when a fresher
 *  one arrives. Each call captures `mySeq` and bails when superseded. */
let _seq = 0;

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
 * @param {Omit<BuildOpts, "floodPx">} layerOpts
 * @param {(message: string) => void} [onProgress] optional progress callback
 */
export async function buildScene(initFloodPx, layerOpts, onProgress) {
  _floodPx = initFloodPx;
  const opts = { ..._withFlood(layerOpts) };
  const cxSteps = Math.ceil(MAP_W / CHUNK_M);
  const czSteps = Math.ceil(MAP_D / CHUNK_M);
  const total = cxSteps * czSteps;
  const parts = [];

  for (let cz = 0; cz < czSteps; cz++) {
    for (let cx = 0; cx < cxSteps; cx++) {
      parts.push(buildChunk(cx, cz, opts));
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
 * Apply a new flood frame, updating the cached buffer.
 * @param {Uint8ClampedArray | null} floodPx
 * @param {Omit<BuildOpts, "floodPx">} layerOpts
 */
export function setFrame(floodPx, layerOpts) {
  _floodPx = floodPx;
  return _applyOpts(_withFlood(layerOpts));
}

/**
 * Re-apply the current frame with new layer toggles. No PNG fetch happens —
 * we use the cached `_floodPx` so toggling is instantaneous.
 * @param {Omit<BuildOpts, "floodPx">} layerOpts
 */
export function setLayers(layerOpts) {
  return _applyOpts(_withFlood(layerOpts));
}

function _withFlood(layerOpts) {
  return { ...layerOpts, floodPx: _floodPx };
}

/**
 * Walk the cached chunk nodes and rewrite their height/colour attributes to
 * reflect `opts`. Cancellable: a later call with a fresher request supersedes
 * any in-flight call.
 * @param {BuildOpts} opts
 */
async function _applyOpts(opts) {
  const mySeq = ++_seq;
  const cxSteps = Math.ceil(MAP_W / CHUNK_M);
  const czSteps = Math.ceil(MAP_D / CHUNK_M);

  for (let czi = 0; czi < czSteps; czi++) {
    for (let cxi = 0; cxi < cxSteps; cxi++) {
      if (mySeq !== _seq) return;  // superseded

      for (const cellM of LOD_SIZES) {
        const def = `C${cxi}_${czi}_${cellM}`;
        const grid = _gridNodes.get(def);
        const col = _colorNodes.get(def + "_C");
        if (!grid || !col) continue;

        const data = buildLodShape(cxi, czi, cellM, opts);
        grid.setAttribute("height", data.heights.join(" "));
        col.setAttribute("color", data.colors.join(" "));
      }
    }
    await new Promise(r => setTimeout(r, 0));
  }
}
