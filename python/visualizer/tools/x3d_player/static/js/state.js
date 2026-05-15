// Reactive state container — single source of truth for the player UI.
//
// Modules call `update({...patch})` to mutate state; subscribers registered
// via `subscribe(fn)` receive the new state on every change. This is a
// minimal Redux-like pattern, no framework needed.
//
// In Phase 2 only playback uses this; later phases (layer toggles, camera
// presets, minimap zone) will hang their state off the same object.

/** @typedef {{
 *   terrain: boolean,
 *   water: boolean,
 *   states: boolean[],
 * }} LayerState */

/** @typedef {{
 *   frame: number,
 *   playing: boolean,
 *   layers: LayerState,
 * }} State */

/**
 * Default layer state: everything visible. `states[0]` is the dry slot and is
 * never read — it lives at index 0 so palette indices map directly.
 * @returns {LayerState}
 */
export function defaultLayers() {
  return {
    terrain: true,
    water: true,
    states: [false, true, true, true, true, true],
  };
}

/** @type {State} */
export const state = {
  frame: 0,
  playing: false,
  layers: defaultLayers(),
};

/** @type {Set<(state: State) => void>} */
const listeners = new Set();

/**
 * Subscribe to state changes. Returns an unsubscribe function.
 * @param {(state: State) => void} fn
 * @returns {() => void}
 */
export function subscribe(fn) {
  listeners.add(fn);
  return () => listeners.delete(fn);
}

/**
 * Apply a partial update and notify subscribers.
 * @param {Partial<State>} patch
 */
export function update(patch) {
  Object.assign(state, patch);
  for (const fn of listeners) fn(state);
}
