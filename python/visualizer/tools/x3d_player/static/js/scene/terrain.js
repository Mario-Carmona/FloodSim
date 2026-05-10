// Terrain heightmap — decoding the embedded PNG into a height-lookup grid
// and providing the colour ramp used for dry cells.

import { PNG_COLS, PNG_ROWS, MIN_H, MAX_H, TERRAIN_SRC } from "../runtime.js";

let _terrainPx = null;

/**
 * Decode an image URL into a flat RGBA pixel buffer (Uint8ClampedArray).
 * @param {string} src
 * @returns {Promise<Uint8ClampedArray>}
 */
async function _decodeImage(src) {
  const img = new Image();
  await new Promise(r => { img.onload = r; img.src = src; });
  const c = document.createElement("canvas");
  c.width = img.width;
  c.height = img.height;
  c.getContext("2d").drawImage(img, 0, 0);
  return c.getContext("2d").getImageData(0, 0, img.width, img.height).data;
}

/**
 * Load and decode the terrain PNG embedded in the page. Must be called once
 * before `getTerrainH` / `terrainColor` are usable.
 */
export async function loadTerrain() {
  _terrainPx = await _decodeImage(TERRAIN_SRC);
}

/**
 * Sample the terrain elevation in metres at PNG pixel coordinates.
 * Out-of-range values are clamped to the last row/column.
 * @param {number} px
 * @param {number} pz
 * @returns {number}
 */
export function getTerrainH(px, pz) {
  if (px >= PNG_COLS) px = PNG_COLS - 1;
  if (pz >= PNG_ROWS) pz = PNG_ROWS - 1;
  return (_terrainPx[(pz * PNG_COLS + px) * 4] / 255.0) * (MAX_H - MIN_H) + MIN_H;
}

/**
 * Discrete terrain colour ramp by elevation. Returns a "r g b" floats string
 * suitable for X3D Color nodes.
 * @param {number} h height in metres
 * @returns {string}
 */
export function terrainColor(h) {
  if (h <= 0)   return "0.20 0.39 0.70";
  if (h <= 5)   return "0.82 0.76 0.57";
  if (h <= 30)  return "0.73 0.80 0.51";
  if (h <= 100) return "0.55 0.67 0.36";
  return "0.51 0.44 0.29";
}
