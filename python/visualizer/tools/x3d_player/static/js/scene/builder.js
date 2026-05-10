// Pure geometry builder — produces height/colour arrays for a single LOD
// chunk. No DOM access, no fetch, no global state: trivially unit-testable.

import { CHUNK_M, LOD_RANGES, LOD_SIZES, WATER_LIFT } from "../config.js";
import { MAP_W, MAP_D, PNG_COLS, PNG_ROWS, PNG_RES, STATE_COLORS } from "../runtime.js";
import { getTerrainH, terrainColor } from "./terrain.js";

/** Sentinel height (metres) used to push hidden cells well below the camera. */
const _HIDDEN_Z = -99999;

/**
 * @typedef {{
 *   floodPx: Uint8ClampedArray | null,
 *   showTerrain: boolean,
 *   showWater: boolean,
 *   stateMask: boolean[],
 * }} BuildOpts
 */

/**
 * @typedef {{
 *   def: string,
 *   pts_x: number,
 *   pts_z: number,
 *   cellM: number,
 *   heights: string[],
 *   colors: string[],
 * }} LodShape
 */

/**
 * Build the height/colour arrays for one LOD level inside one chunk.
 *
 * Cell visibility rules:
 *   - If `showWater` is false OR `floodPx` is null OR the cell's palette
 *     state is masked off, the cell is treated as dry.
 *   - A dry cell with `showTerrain=false` is hidden by sinking it to a very
 *     negative Z (the mesh stays continuous, the camera just doesn't see it).
 *   - A flooded cell shows the palette colour for its (unmasked) state and
 *     is lifted by `WATER_LIFT[state]` metres.
 *
 * @param {number} cxi          chunk index along X
 * @param {number} czi          chunk index along Z
 * @param {number} cellM        mesh cell size in metres for this LOD
 * @param {BuildOpts} opts
 * @returns {LodShape}
 */
export function buildLodShape(cxi, czi, cellM, opts) {
  const { floodPx, showTerrain, showWater, stateMask } = opts;
  const tx = cxi * CHUNK_M;
  const tz = czi * CHUNK_M;
  const aw = Math.min(CHUNK_M, MAP_W - tx);
  const ad = Math.min(CHUNK_M, MAP_D - tz);
  const pts_x = Math.floor(aw / cellM) + 1;
  const pts_z = Math.floor(ad / cellM) + 1;
  const heights = [];
  const colors = [];

  for (let lz = 0; lz < pts_z; lz++) {
    for (let lx = 0; lx < pts_x; lx++) {
      const px = Math.min(Math.floor((tx + lx * cellM) / PNG_RES), PNG_COLS - 1);
      const pz = Math.min(Math.floor((tz + lz * cellM) / PNG_RES), PNG_ROWS - 1);
      const h = getTerrainH(px, pz);

      const rawSt = (showWater && floodPx) ? floodPx[(pz * PNG_COLS + px) * 4] : 0;
      // Cells whose state is masked off (or whose state is 0) are dry.
      const flooded = rawSt > 0 && stateMask[Math.min(rawSt, stateMask.length - 1)];

      if (flooded) {
        const st = Math.min(rawSt, WATER_LIFT.length - 1);
        heights.push((h + WATER_LIFT[st]).toFixed(2));
        colors.push(STATE_COLORS[Math.min(rawSt, STATE_COLORS.length - 1)]);
      } else if (showTerrain) {
        heights.push(h.toFixed(2));
        colors.push(terrainColor(h));
      } else {
        heights.push(_HIDDEN_Z.toFixed(2));
        colors.push(terrainColor(h));  // colour irrelevant — vertex out of view
      }
    }
  }

  const def = `C${cxi}_${czi}_${cellM}`;
  return { def, pts_x, pts_z, cellM, heights, colors };
}

/**
 * Wrap a built LOD shape in the X3D markup x_ite consumes.
 * @param {LodShape} shape
 * @returns {string}
 */
export function shapeHTML({ def, pts_x, pts_z, cellM, heights, colors }) {
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

/**
 * Build the full markup for one chunk: a Transform wrapping an LOD that
 * contains one Shape per LOD_SIZES level.
 * @param {number} cxi
 * @param {number} czi
 * @param {BuildOpts} opts
 * @returns {string}
 */
export function buildChunk(cxi, czi, opts) {
  const tx = cxi * CHUNK_M;
  const tz = czi * CHUNK_M;
  const aw = Math.min(CHUNK_M, MAP_W - tx);
  const ad = Math.min(CHUNK_M, MAP_D - tz);
  let lodShapes = "";

  for (const cellM of LOD_SIZES) {
    lodShapes += shapeHTML(buildLodShape(cxi, czi, cellM, opts));
  }
  lodShapes += '<WorldInfo info="fuera_de_rango"/>';

  return `<Transform translation="${tx} 0 ${tz}">
    <LOD range="${LOD_RANGES}" center="${(aw / 2).toFixed(1)} 0 ${(ad / 2).toFixed(1)}">
      ${lodShapes}
    </LOD>
  </Transform>`;
}
