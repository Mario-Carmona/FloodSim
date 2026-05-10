// Pure geometry builder — produces height/colour arrays for a single LOD
// chunk. No DOM access, no fetch, no global state: trivially unit-testable.

import { CHUNK_M, LOD_RANGES, LOD_SIZES, WATER_LIFT } from "../config.js";
import { MAP_W, MAP_D, PNG_COLS, PNG_ROWS, PNG_RES, STATE_COLORS } from "../runtime.js";
import { getTerrainH, terrainColor } from "./terrain.js";

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
 * Returned shape is later wrapped in X3D markup by `shapeHTML`.
 *
 * @param {number} cxi          chunk index along X
 * @param {number} czi          chunk index along Z
 * @param {number} cellM        mesh cell size in metres for this LOD
 * @param {boolean} terrainOnly skip flood overlay (used during initial bake)
 * @param {Uint8ClampedArray | null} floodPx decoded flood RGBA pixels
 * @returns {LodShape}
 */
export function buildLodShape(cxi, czi, cellM, terrainOnly, floodPx) {
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
      const st = (!terrainOnly && floodPx) ? floodPx[(pz * PNG_COLS + px) * 4] : 0;
      const lift = st > 0 ? WATER_LIFT[Math.min(st, WATER_LIFT.length - 1)] : 0;
      heights.push((h + lift).toFixed(2));
      colors.push(st > 0 ? STATE_COLORS[Math.min(st, STATE_COLORS.length - 1)] : terrainColor(h));
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
 * @param {Uint8ClampedArray | null} floodPx
 * @returns {string}
 */
export function buildChunk(cxi, czi, floodPx) {
  const tx = cxi * CHUNK_M;
  const tz = czi * CHUNK_M;
  const aw = Math.min(CHUNK_M, MAP_W - tx);
  const ad = Math.min(CHUNK_M, MAP_D - tz);
  let lodShapes = "";

  for (const cellM of LOD_SIZES) {
    lodShapes += shapeHTML(buildLodShape(cxi, czi, cellM, false, floodPx));
  }
  lodShapes += '<WorldInfo info="fuera_de_rango"/>';

  return `<Transform translation="${tx} 0 ${tz}">
    <LOD range="${LOD_RANGES}" center="${(aw / 2).toFixed(1)} 0 ${(ad / 2).toFixed(1)}">
      ${lodShapes}
    </LOD>
  </Transform>`;
}
