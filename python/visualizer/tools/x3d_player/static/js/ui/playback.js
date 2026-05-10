// Playback controls — wires the slider, prev/next/play buttons and the
// speed slider to the state container and the scene update pipeline.
//
// Uses state.js for `frame` and `playing`; the DOM is the source of truth
// for `speed` since it's read-only from JS's perspective.

import { FLOOD_FRAMES } from "../runtime.js";
import { fetchFloodPx } from "../scene/flood.js";
import { applyFlood } from "../scene/scene.js";
import { state, update } from "../state.js";

let _timer = null;

function _frameLabel(i) {
  return `Frame ${i} / ${FLOOD_FRAMES.length - 1}`;
}

/**
 * Show frame `i` (modulo the frame list). If it differs from the currently
 * displayed frame, fetch the flood PNG and apply it to the scene.
 * @param {number} i
 */
async function setFrame(i) {
  const n = FLOOD_FRAMES.length;
  const next = ((i % n) + n) % n;
  document.getElementById("slider").value = next;
  document.getElementById("frame-label").textContent = _frameLabel(next);

  if (next === state.frame) return;  // already showing this frame
  update({ frame: next });

  const floodPx = await fetchFloodPx(FLOOD_FRAMES[next]);
  applyFlood(floodPx);
}

function startPlay() {
  const ms = parseInt(document.getElementById("speed").value);
  _timer = setInterval(() => setFrame(state.frame + 1), ms);
  update({ playing: true });
  document.getElementById("playBtn").textContent = "⏸ Pause";
}

function stopPlay() {
  clearInterval(_timer);
  _timer = null;
  update({ playing: false });
  document.getElementById("playBtn").textContent = "▶ Play";
}

/**
 * Wire up event listeners. Must be called once after the scene is ready.
 */
export function initPlayback() {
  document.getElementById("playBtn").addEventListener("click",
    () => state.playing ? stopPlay() : startPlay());

  document.getElementById("prevBtn").addEventListener("click",
    () => { stopPlay(); setFrame(state.frame - 1); });

  document.getElementById("nextBtn").addEventListener("click",
    () => { stopPlay(); setFrame(state.frame + 1); });

  document.getElementById("slider").addEventListener("input",
    e => { stopPlay(); setFrame(parseInt(e.target.value)); });

  document.getElementById("speed").addEventListener("change",
    () => { if (state.playing) { stopPlay(); startPlay(); } });
}
